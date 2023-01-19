# Microsoft Developer Studio Generated NMAKE File, Based on cavass.dsp
!IF "$(CFG)" == ""
CFG=cavass - Win32 Debug
!MESSAGE No configuration specified. Defaulting to cavass - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "cavass - Win32 Release" && "$(CFG)" != "cavass - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "cavass - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\cavass.exe"


CLEAN :
	-@erase "$(INTDIR)\AtoM.obj"
	-@erase "$(INTDIR)\cavass.res"
	-@erase "$(INTDIR)\cmput_colrs.obj"
	-@erase "$(INTDIR)\Dicom.obj"
	-@erase "$(INTDIR)\ExampleCanvas.obj"
	-@erase "$(INTDIR)\ExampleFrame.obj"
	-@erase "$(INTDIR)\fft.obj"
	-@erase "$(INTDIR)\FileHistory.obj"
	-@erase "$(INTDIR)\FuzzCompCanvas.obj"
	-@erase "$(INTDIR)\FuzzCompFrame.obj"
	-@erase "$(INTDIR)\GaussianTransform.obj"
	-@erase "$(INTDIR)\gcode.obj"
	-@erase "$(INTDIR)\glob_vars.obj"
	-@erase "$(INTDIR)\GrayScaleTransform.obj"
	-@erase "$(INTDIR)\InformationDialog.obj"
	-@erase "$(INTDIR)\InputHistoryDialog.obj"
	-@erase "$(INTDIR)\InterpolateCanvas.obj"
	-@erase "$(INTDIR)\InterpolateFrame.obj"
	-@erase "$(INTDIR)\load.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\MainCanvas.obj"
	-@erase "$(INTDIR)\MainFrame.obj"
	-@erase "$(INTDIR)\MainSplitter.obj"
	-@erase "$(INTDIR)\make_image.obj"
	-@erase "$(INTDIR)\manip_error.obj"
	-@erase "$(INTDIR)\matrix.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\MontageCanvas.obj"
	-@erase "$(INTDIR)\MontageFrame.obj"
	-@erase "$(INTDIR)\obj_number.obj"
	-@erase "$(INTDIR)\obj_transf.obj"
	-@erase "$(INTDIR)\OverlayCanvas.obj"
	-@erase "$(INTDIR)\OverlayFrame.obj"
	-@erase "$(INTDIR)\patch.obj"
	-@erase "$(INTDIR)\Preferences.obj"
	-@erase "$(INTDIR)\PreferencesDialog.obj"
	-@erase "$(INTDIR)\project.obj"
	-@erase "$(INTDIR)\RampTransform.obj"
	-@erase "$(INTDIR)\reflect.obj"
	-@erase "$(INTDIR)\RegisterFrame.obj"
	-@erase "$(INTDIR)\render.obj"
	-@erase "$(INTDIR)\salloc.obj"
	-@erase "$(INTDIR)\shade.obj"
	-@erase "$(INTDIR)\show_objects.obj"
	-@erase "$(INTDIR)\SurfViewCanvas.obj"
	-@erase "$(INTDIR)\SurfViewFrame.obj"
	-@erase "$(INTDIR)\ThresholdCanvas.obj"
	-@erase "$(INTDIR)\ThresholdFrame.obj"
	-@erase "$(INTDIR)\TrapezoidTransform.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\view.obj"
	-@erase "$(INTDIR)\view_interp.obj"
	-@erase "$(OUTDIR)\cavass.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\cavass.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\cavass.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\cavass.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib 3dviewnix.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\cavass.pdb" /machine:I386 /out:"$(OUTDIR)\cavass.exe" 
LINK32_OBJS= \
	"$(INTDIR)\AtoM.obj" \
	"$(INTDIR)\cmput_colrs.obj" \
	"$(INTDIR)\Dicom.obj" \
	"$(INTDIR)\ExampleCanvas.obj" \
	"$(INTDIR)\ExampleFrame.obj" \
	"$(INTDIR)\fft.obj" \
	"$(INTDIR)\FileHistory.obj" \
	"$(INTDIR)\FuzzCompCanvas.obj" \
	"$(INTDIR)\FuzzCompFrame.obj" \
	"$(INTDIR)\GaussianTransform.obj" \
	"$(INTDIR)\gcode.obj" \
	"$(INTDIR)\glob_vars.obj" \
	"$(INTDIR)\GrayScaleTransform.obj" \
	"$(INTDIR)\InformationDialog.obj" \
	"$(INTDIR)\InputHistoryDialog.obj" \
	"$(INTDIR)\InterpolateCanvas.obj" \
	"$(INTDIR)\InterpolateFrame.obj" \
	"$(INTDIR)\load.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\MainCanvas.obj" \
	"$(INTDIR)\MainFrame.obj" \
	"$(INTDIR)\MainSplitter.obj" \
	"$(INTDIR)\make_image.obj" \
	"$(INTDIR)\manip_error.obj" \
	"$(INTDIR)\matrix.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\MontageCanvas.obj" \
	"$(INTDIR)\MontageFrame.obj" \
	"$(INTDIR)\obj_number.obj" \
	"$(INTDIR)\obj_transf.obj" \
	"$(INTDIR)\OverlayCanvas.obj" \
	"$(INTDIR)\OverlayFrame.obj" \
	"$(INTDIR)\patch.obj" \
	"$(INTDIR)\Preferences.obj" \
	"$(INTDIR)\PreferencesDialog.obj" \
	"$(INTDIR)\project.obj" \
	"$(INTDIR)\RampTransform.obj" \
	"$(INTDIR)\reflect.obj" \
	"$(INTDIR)\RegisterFrame.obj" \
	"$(INTDIR)\render.obj" \
	"$(INTDIR)\salloc.obj" \
	"$(INTDIR)\shade.obj" \
	"$(INTDIR)\show_objects.obj" \
	"$(INTDIR)\SurfViewCanvas.obj" \
	"$(INTDIR)\SurfViewFrame.obj" \
	"$(INTDIR)\ThresholdCanvas.obj" \
	"$(INTDIR)\ThresholdFrame.obj" \
	"$(INTDIR)\TrapezoidTransform.obj" \
	"$(INTDIR)\view.obj" \
	"$(INTDIR)\view_interp.obj" \
	"$(INTDIR)\cavass.res"

"$(OUTDIR)\cavass.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "cavass - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\cavass.exe"


CLEAN :
	-@erase "$(INTDIR)\AtoM.obj"
	-@erase "$(INTDIR)\cavass.res"
	-@erase "$(INTDIR)\cmput_colrs.obj"
	-@erase "$(INTDIR)\Dicom.obj"
	-@erase "$(INTDIR)\ExampleCanvas.obj"
	-@erase "$(INTDIR)\ExampleFrame.obj"
	-@erase "$(INTDIR)\fft.obj"
	-@erase "$(INTDIR)\FileHistory.obj"
	-@erase "$(INTDIR)\FuzzCompCanvas.obj"
	-@erase "$(INTDIR)\FuzzCompFrame.obj"
	-@erase "$(INTDIR)\GaussianTransform.obj"
	-@erase "$(INTDIR)\gcode.obj"
	-@erase "$(INTDIR)\glob_vars.obj"
	-@erase "$(INTDIR)\GrayScaleTransform.obj"
	-@erase "$(INTDIR)\InformationDialog.obj"
	-@erase "$(INTDIR)\InputHistoryDialog.obj"
	-@erase "$(INTDIR)\InterpolateCanvas.obj"
	-@erase "$(INTDIR)\InterpolateFrame.obj"
	-@erase "$(INTDIR)\load.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\MainCanvas.obj"
	-@erase "$(INTDIR)\MainFrame.obj"
	-@erase "$(INTDIR)\MainSplitter.obj"
	-@erase "$(INTDIR)\make_image.obj"
	-@erase "$(INTDIR)\manip_error.obj"
	-@erase "$(INTDIR)\matrix.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\MontageCanvas.obj"
	-@erase "$(INTDIR)\MontageFrame.obj"
	-@erase "$(INTDIR)\obj_number.obj"
	-@erase "$(INTDIR)\obj_transf.obj"
	-@erase "$(INTDIR)\OverlayCanvas.obj"
	-@erase "$(INTDIR)\OverlayFrame.obj"
	-@erase "$(INTDIR)\patch.obj"
	-@erase "$(INTDIR)\Preferences.obj"
	-@erase "$(INTDIR)\PreferencesDialog.obj"
	-@erase "$(INTDIR)\project.obj"
	-@erase "$(INTDIR)\RampTransform.obj"
	-@erase "$(INTDIR)\reflect.obj"
	-@erase "$(INTDIR)\RegisterFrame.obj"
	-@erase "$(INTDIR)\render.obj"
	-@erase "$(INTDIR)\salloc.obj"
	-@erase "$(INTDIR)\shade.obj"
	-@erase "$(INTDIR)\show_objects.obj"
	-@erase "$(INTDIR)\SurfViewCanvas.obj"
	-@erase "$(INTDIR)\SurfViewFrame.obj"
	-@erase "$(INTDIR)\ThresholdCanvas.obj"
	-@erase "$(INTDIR)\ThresholdFrame.obj"
	-@erase "$(INTDIR)\TrapezoidTransform.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\view.obj"
	-@erase "$(INTDIR)\view_interp.obj"
	-@erase "$(OUTDIR)\cavass.exe"
	-@erase "$(OUTDIR)\cavass.ilk"
	-@erase "$(OUTDIR)\cavass.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GR /GX /ZI /Od /I "." /I "frames" /I "..\wxWidgets-2.6.3\lib\vc_lib\msw" /I "..\wxWidgets-2.6.3\include" /I "..\3dviewnix\INCLUDE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\cavass.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\cavass.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\cavass.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib rpcrt4.lib wsock32.lib wxmsw26d_adv.lib wxmsw26d_html.lib wxmsw26d_core.lib wxbase26d.lib wxtiffd.lib wxjpegd.lib wxpngd.lib wxzlibd.lib wxregexd.lib wxexpatd.lib 3dviewnix.lib /nologo /stack:0x800000 /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\cavass.pdb" /debug /machine:I386 /nodefaultlib:"libcd.lib" /out:"$(OUTDIR)\cavass.exe" /pdbtype:sept /libpath:"..\wxWidgets-2.6.3\lib\vc_lib" /libpath:"..\wxWidgets-2.6.3\contrib\lib" /libpath:"..\3dviewnix\Debug" 
LINK32_OBJS= \
	"$(INTDIR)\AtoM.obj" \
	"$(INTDIR)\cmput_colrs.obj" \
	"$(INTDIR)\Dicom.obj" \
	"$(INTDIR)\ExampleCanvas.obj" \
	"$(INTDIR)\ExampleFrame.obj" \
	"$(INTDIR)\fft.obj" \
	"$(INTDIR)\FileHistory.obj" \
	"$(INTDIR)\FuzzCompCanvas.obj" \
	"$(INTDIR)\FuzzCompFrame.obj" \
	"$(INTDIR)\GaussianTransform.obj" \
	"$(INTDIR)\gcode.obj" \
	"$(INTDIR)\glob_vars.obj" \
	"$(INTDIR)\GrayScaleTransform.obj" \
	"$(INTDIR)\InformationDialog.obj" \
	"$(INTDIR)\InputHistoryDialog.obj" \
	"$(INTDIR)\InterpolateCanvas.obj" \
	"$(INTDIR)\InterpolateFrame.obj" \
	"$(INTDIR)\load.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\MainCanvas.obj" \
	"$(INTDIR)\MainFrame.obj" \
	"$(INTDIR)\MainSplitter.obj" \
	"$(INTDIR)\make_image.obj" \
	"$(INTDIR)\manip_error.obj" \
	"$(INTDIR)\matrix.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\MontageCanvas.obj" \
	"$(INTDIR)\MontageFrame.obj" \
	"$(INTDIR)\obj_number.obj" \
	"$(INTDIR)\obj_transf.obj" \
	"$(INTDIR)\OverlayCanvas.obj" \
	"$(INTDIR)\OverlayFrame.obj" \
	"$(INTDIR)\patch.obj" \
	"$(INTDIR)\Preferences.obj" \
	"$(INTDIR)\PreferencesDialog.obj" \
	"$(INTDIR)\project.obj" \
	"$(INTDIR)\RampTransform.obj" \
	"$(INTDIR)\reflect.obj" \
	"$(INTDIR)\RegisterFrame.obj" \
	"$(INTDIR)\render.obj" \
	"$(INTDIR)\salloc.obj" \
	"$(INTDIR)\shade.obj" \
	"$(INTDIR)\show_objects.obj" \
	"$(INTDIR)\SurfViewCanvas.obj" \
	"$(INTDIR)\SurfViewFrame.obj" \
	"$(INTDIR)\ThresholdCanvas.obj" \
	"$(INTDIR)\ThresholdFrame.obj" \
	"$(INTDIR)\TrapezoidTransform.obj" \
	"$(INTDIR)\view.obj" \
	"$(INTDIR)\view_interp.obj" \
	"$(INTDIR)\cavass.res"

"$(OUTDIR)\cavass.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("cavass.dep")
!INCLUDE "cavass.dep"
!ELSE 
!MESSAGE Warning: cannot find "cavass.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "cavass - Win32 Release" || "$(CFG)" == "cavass - Win32 Debug"
SOURCE=.\render\AtoM.cpp

"$(INTDIR)\AtoM.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\cavass.rc

"$(INTDIR)\cavass.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\render\cmput_colrs.cpp

"$(INTDIR)\cmput_colrs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Dicom.cpp

"$(INTDIR)\Dicom.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\frames\ExampleCanvas.cpp

"$(INTDIR)\ExampleCanvas.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\ExampleFrame.cpp

"$(INTDIR)\ExampleFrame.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\fft.cpp

"$(INTDIR)\fft.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\FileHistory.cpp

"$(INTDIR)\FileHistory.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\frames\FuzzCompCanvas.cpp

"$(INTDIR)\FuzzCompCanvas.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\FuzzCompFrame.cpp

"$(INTDIR)\FuzzCompFrame.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\GaussianTransform.cpp

"$(INTDIR)\GaussianTransform.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\render\gcode.cpp

"$(INTDIR)\gcode.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\glob_vars.cpp

"$(INTDIR)\glob_vars.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\GrayScaleTransform.cpp

"$(INTDIR)\GrayScaleTransform.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\frames\InformationDialog.cpp

"$(INTDIR)\InformationDialog.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\InputHistoryDialog.cpp

"$(INTDIR)\InputHistoryDialog.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\InterpolateCanvas.cpp

"$(INTDIR)\InterpolateCanvas.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\InterpolateFrame.cpp

"$(INTDIR)\InterpolateFrame.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\load.cpp

"$(INTDIR)\load.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\main.cpp

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\frames\MainCanvas.cpp

"$(INTDIR)\MainCanvas.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\MainFrame.cpp

"$(INTDIR)\MainFrame.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\MainSplitter.cpp

"$(INTDIR)\MainSplitter.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\make_image.cpp

"$(INTDIR)\make_image.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\manip_error.cpp

"$(INTDIR)\manip_error.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\matrix.cpp

"$(INTDIR)\matrix.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\misc.cpp

"$(INTDIR)\misc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\frames\MontageCanvas.cpp

"$(INTDIR)\MontageCanvas.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\MontageFrame.cpp

"$(INTDIR)\MontageFrame.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\obj_number.cpp

"$(INTDIR)\obj_number.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\obj_transf.cpp

"$(INTDIR)\obj_transf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\OverlayCanvas.cpp

"$(INTDIR)\OverlayCanvas.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\OverlayFrame.cpp

"$(INTDIR)\OverlayFrame.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\patch.cpp

"$(INTDIR)\patch.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\Preferences.cpp

"$(INTDIR)\Preferences.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\PreferencesDialog.cpp

"$(INTDIR)\PreferencesDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\render\project.cpp

"$(INTDIR)\project.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\RampTransform.cpp

"$(INTDIR)\RampTransform.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\render\reflect.cpp

"$(INTDIR)\reflect.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\RegisterFrame.cpp

"$(INTDIR)\RegisterFrame.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\render.cpp

"$(INTDIR)\render.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\salloc.cpp

"$(INTDIR)\salloc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\shade.cpp

"$(INTDIR)\shade.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\show_objects.cpp

"$(INTDIR)\show_objects.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\SurfViewCanvas.cpp

"$(INTDIR)\SurfViewCanvas.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\SurfViewFrame.cpp

"$(INTDIR)\SurfViewFrame.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\ThresholdCanvas.cpp

"$(INTDIR)\ThresholdCanvas.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\frames\ThresholdFrame.cpp

"$(INTDIR)\ThresholdFrame.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\TrapezoidTransform.cpp

"$(INTDIR)\TrapezoidTransform.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\render\view.cpp

"$(INTDIR)\view.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\view_interp.cpp

"$(INTDIR)\view_interp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

