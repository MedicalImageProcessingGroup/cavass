# Microsoft Developer Studio Project File - Name="cavass" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=cavass - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cavass.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cavass.mak" CFG="cavass - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cavass - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "cavass - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cavass - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib 3dviewnix.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "cavass - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GR /GX /ZI /Od /I "." /I "frames" /I "..\wxWidgets-2.6.3\lib\vc_lib\msw" /I "..\wxWidgets-2.6.3\include" /I "..\3dviewnix\INCLUDE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib rpcrt4.lib wsock32.lib wxmsw26d_adv.lib wxmsw26d_html.lib wxmsw26d_core.lib wxbase26d.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxregexd.lib wxexpatd.lib 3dviewnix.lib /nologo /stack:0x800000 /subsystem:windows /debug /machine:I386 /nodefaultlib:"libcd.lib" /pdbtype:sept /libpath:"..\wxWidgets-2.6.3\lib\vc_lib" /libpath:"..\wxWidgets-2.6.3\contrib\lib" /libpath:"..\3dviewnix\Debug"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "cavass - Win32 Release"
# Name "cavass - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\render\AtoM.cpp
# End Source File
# Begin Source File

SOURCE=.\cavass.rc
# End Source File
# Begin Source File

SOURCE=.\render\cmput_colrs.cpp
# End Source File
# Begin Source File

SOURCE=.\Dicom.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\ExampleCanvas.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\ExampleFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\fft.cpp
# End Source File
# Begin Source File

SOURCE=.\FileHistory.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\FuzzCompCanvas.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\FuzzCompFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\GaussianTransform.cpp
# End Source File
# Begin Source File

SOURCE=.\render\gcode.cpp
# End Source File
# Begin Source File

SOURCE=.\render\glob_vars.cpp
# End Source File
# Begin Source File

SOURCE=.\GrayScaleTransform.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\InformationDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\InputHistoryDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\InterpolateCanvas.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\InterpolateFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\render\load.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\MainCanvas.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\MainFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\MainSplitter.cpp
# End Source File
# Begin Source File

SOURCE=.\render\make_image.cpp
# End Source File
# Begin Source File

SOURCE=.\render\manip_error.cpp
# End Source File
# Begin Source File

SOURCE=.\render\matrix.cpp
# End Source File
# Begin Source File

SOURCE=.\misc.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\MontageCanvas.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\MontageFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\render\obj_number.cpp
# End Source File
# Begin Source File

SOURCE=.\render\obj_transf.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\OverlayCanvas.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\OverlayFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\render\patch.cpp
# End Source File
# Begin Source File

SOURCE=.\Preferences.cpp
# End Source File
# Begin Source File

SOURCE=.\PreferencesDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\render\project.cpp
# End Source File
# Begin Source File

SOURCE=.\RampTransform.cpp
# End Source File
# Begin Source File

SOURCE=.\render\reflect.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\RegisterFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\render\render.cpp
# End Source File
# Begin Source File

SOURCE=.\render\salloc.cpp
# End Source File
# Begin Source File

SOURCE=.\render\shade.cpp
# End Source File
# Begin Source File

SOURCE=.\render\show_objects.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\SurfViewCanvas.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\SurfViewFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\ThresholdCanvas.cpp
# End Source File
# Begin Source File

SOURCE=.\frames\ThresholdFrame.cpp
# End Source File
# Begin Source File

SOURCE=.\TrapezoidTransform.cpp
# End Source File
# Begin Source File

SOURCE=.\render\view.cpp
# End Source File
# Begin Source File

SOURCE=.\render\view_interp.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cavass.h
# End Source File
# Begin Source File

SOURCE=.\CavassData.h
# End Source File
# Begin Source File

SOURCE=.\frames\CineControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\ClassifyControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\ColorControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\CovarianceControls.h
# End Source File
# Begin Source File

SOURCE=.\render\cursorfont.h
# End Source File
# Begin Source File

SOURCE=.\frames\ExampleCanvas.h
# End Source File
# Begin Source File

SOURCE=.\frames\ExampleFrame.h
# End Source File
# Begin Source File

SOURCE=.\fft.h
# End Source File
# Begin Source File

SOURCE=.\FileHistory.h
# End Source File
# Begin Source File

SOURCE=.\frames\FunctionControls.h
# End Source File
# Begin Source File

SOURCE=.\render\functions.h
# End Source File
# Begin Source File

SOURCE=.\frames\FuzzCompCanvas.h
# End Source File
# Begin Source File

SOURCE=.\frames\FuzzCompFrame.h
# End Source File
# Begin Source File

SOURCE=.\GaussianTransform.h
# End Source File
# Begin Source File

SOURCE=.\render\glob_vars.h
# End Source File
# Begin Source File

SOURCE=.\Globals.h
# End Source File
# Begin Source File

SOURCE=.\frames\GrayMapControls.h
# End Source File
# Begin Source File

SOURCE=.\GrayScaleTransform.h
# End Source File
# Begin Source File

SOURCE=.\frames\HistoSettingControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\InformationDialog.h
# End Source File
# Begin Source File

SOURCE=.\frames\InputHistoryDialog.h
# End Source File
# Begin Source File

SOURCE=.\frames\InterpolateCanvas.h
# End Source File
# Begin Source File

SOURCE=.\frames\InterpolateFrame.h
# End Source File
# Begin Source File

SOURCE=.\frames\MagnifyControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\MainCanvas.h
# End Source File
# Begin Source File

SOURCE=.\frames\MainFileDropTarget.h
# End Source File
# Begin Source File

SOURCE=.\frames\MainFrame.h
# End Source File
# Begin Source File

SOURCE=.\frames\MainPrint.h
# End Source File
# Begin Source File

SOURCE=.\render\manip_etc.h
# End Source File
# Begin Source File

SOURCE=.\render\manipulate.h
# End Source File
# Begin Source File

SOURCE=.\frames\MontageCanvas.h
# End Source File
# Begin Source File

SOURCE=.\frames\MontageFrame.h
# End Source File
# Begin Source File

SOURCE=.\frames\MontagePrint.h
# End Source File
# Begin Source File

SOURCE=.\render\neighbors.h
# End Source File
# Begin Source File

SOURCE=.\frames\OpacityControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\OverlayCanvas.h
# End Source File
# Begin Source File

SOURCE=.\frames\OverlayFrame.h
# End Source File
# Begin Source File

SOURCE=.\frames\OverlayPrint.h
# End Source File
# Begin Source File

SOURCE=.\frames\ParameterControls.h
# End Source File
# Begin Source File

SOURCE=.\Preferences.h
# End Source File
# Begin Source File

SOURCE=.\PreferencesDialog.h
# End Source File
# Begin Source File

SOURCE=.\ProcessManager.h
# End Source File
# Begin Source File

SOURCE=.\RampTransform.h
# End Source File
# Begin Source File

SOURCE=.\frames\RegisterFrame.h
# End Source File
# Begin Source File

SOURCE=.\frames\RegistrationControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\SaveScreenControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\SetFuzzCompIndexControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\SetIndexControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\SurfOpacityControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\SurfPropertiesControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\SurfSpeedControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\SurfViewCanvas.h
# End Source File
# Begin Source File

SOURCE=.\frames\SurfViewFrame.h
# End Source File
# Begin Source File

SOURCE=.\frames\SurfViewPrint.h
# End Source File
# Begin Source File

SOURCE=.\frames\ThresholdCanvas.h
# End Source File
# Begin Source File

SOURCE=.\frames\ThresholdControls.h
# End Source File
# Begin Source File

SOURCE=.\frames\ThresholdFrame.h
# End Source File
# Begin Source File

SOURCE=.\frames\ThresholdPrint.h
# End Source File
# Begin Source File

SOURCE=.\TrapezoidTransform.h
# End Source File
# Begin Source File

SOURCE=.\render\Vdataheader.h
# End Source File
# Begin Source File

SOURCE=.\render\Viewnix.h
# End Source File
# Begin Source File

SOURCE=.\render\Vparam.h
# End Source File
# Begin Source File

SOURCE=.\render\Vtypedef.h
# End Source File
# Begin Source File

SOURCE=.\render\X.h
# End Source File
# Begin Source File

SOURCE=.\render\Xlib.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
