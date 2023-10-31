/*
  Copyright 1993-2013, 2015, 2023 Medical Image Processing Group
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
 * \file   Preferences.cpp
 * \brief  Preferences class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"

Preferences*   Preferences::_instance    = 0;
wxConfigBase*  Preferences::_preferences = 0;

int            Preferences::_bgBlue  = 89;
int            Preferences::_bgGreen = 69;
int            Preferences::_bgRed   = 69;
long           Preferences::_customAppearance = 1;
int            Preferences::_fgBlue  = 167;
int            Preferences::_fgGreen = 255;
int            Preferences::_fgRed   = 255;
wxString       Preferences::_file[ Preferences::FileCount ];
wxString       Preferences::_hostNames( "" );
wxString       Preferences::_hostProcessCounts( "" );
wxString       Preferences::_inputDirectory(  "." );
wxString       Preferences::_outputDirectory( "." );
long           Preferences::_parallelMode    = 0;
wxString       Preferences::_saveScreenFileName( "temp.tiff" );
long           Preferences::_showLog         = 0;
long           Preferences::_showSaveScreen  = 0;
long           Preferences::_showToolTips    = 0;
long           Preferences::_singleFrameMode = 1;

long           Preferences::_stereoMode      = StereoModeOff;
double         Preferences::_stereoAngle     = 4.0;
int            Preferences::_stereoLeftRed   = 255,  Preferences::_stereoLeftGreen  =   0,  Preferences::_stereoLeftBlue  =   0;
int            Preferences::_stereoRightRed  = 0,    Preferences::_stereoRightGreen = 255,  Preferences::_stereoRightBlue = 255;
int            Preferences::_stereoLeftOdd   = 1;

long           Preferences::_useInputHistory = 1;

int            Preferences::_CTLungCenter    = 550;
int            Preferences::_CTLungWidth     = 1730;
int            Preferences::_CTSoftTissueCenter = 1000;
int            Preferences::_CTSoftTissueWidth  = 500;
int            Preferences::_CTBoneCenter    = 2000;
int            Preferences::_CTBoneWidth     = 4000;
int            Preferences::_PETCenter    = 1200;
int            Preferences::_PETWidth     = 3500;

wxString       Preferences::_OverlayScale = "1.5";
int            Preferences::_IM0onIM0Red   = 100;
int            Preferences::_IM0onIM0Green = 50;
int            Preferences::_IM0onIM0Blue  = 0;
int            Preferences::_BIMonIM0Red   = 0;
int            Preferences::_BIMonIM0Green = 25;
int            Preferences::_BIMonIM0Blue  = 0;
int            Preferences::_BIMonBIMRed   = 100;
int            Preferences::_BIMonBIMGreen = 50;
int            Preferences::_BIMonBIMBlue  = 0;


#if defined (WIN32) || defined (_WIN32)
    wxString   Preferences::_home( "c:/cavass-build/debug" );
    wxString   Preferences::_MPIDirectory( "c:/Program Files/MPICH2/bin" );
#else
    wxString   Preferences::_home( "." );
    wxString   Preferences::_MPIDirectory( "" );
#endif
//----------------------------------------------------------------------
/** \brief This function sets the VIEWNIX_ENV and PATH enviromnent vars
 *  according to the CAVASS home preference value.  This function also
 *  caches argv[0] which contains the name of the cavass executable
 *  file.  Whenever CAVASS home is changed, this function is/should be
 *  called to update VIEWNIX_ENV and PATH.  At that time the path to the
 *  CAVASS executable is compared with the new CAVASS home.  If they are
 *  different, then the user will be warned of a potential problem.
 *  \todo This should (but doesn't) work with relative path names.
 */
void modifyEnvironment ( char* cavassExe ) {
    static char*  argv0 = NULL;

    if (cavassExe!=NULL)
	{
	    if (argv0)
			free(argv0);
		argv0 = (char *)malloc(strlen(cavassExe)+1);
		strcpy(argv0, cavassExe);
    }
    printf( "VIEWNIX_ENV currently set to %s \n", getenv("VIEWNIX_ENV") );
    printf( "PATH        currently set to %s \n", getenv("PATH") );
#if defined (WIN32) || defined (_WIN32)
    puts( "setting VIEWNIX_ENV" );
    wxString  tmp = wxString::Format( "VIEWNIX_ENV=%s",
                                (const char *)Preferences::getHome().c_str() );
    putenv( (char*)(const char *)tmp.c_str() );

    puts( "setting PATH" );
    tmp = wxString::Format( "PATH=%s;%s", (const char *)Preferences::getHome().c_str(),
                                          getenv("PATH") );
    putenv( (char*)(const char *)tmp.c_str() );

    //determine if the CAVASS home dir and the dir of the cavass
    // executable are equivalent
    wxFileName  fn1( Preferences::getHome() );
    fn1.MakeAbsolute();
    wxString  s1 = fn1.GetFullPath();
    s1.LowerCase();

    wxFileName  fn2( argv0 );
    fn2.MakeAbsolute();
    wxString  s2 = fn2.GetPath();
    s2.LowerCase();

    if (s1.Cmp(s2)!=0) {
        wxString  tmp = wxString::Format(
            "CAVASS is currently executing out of the %s folder.\n\nBut CAVASS home is set to %s.\n\nThis may cause problems running CAVASS.\n\nChange CAVASS home to %s now?\n(You may also view change preferences via Edit->Preferences->Directories.)",
            (const char *)s2.c_str(), (const char *)s1.c_str(), (const char *)s2.c_str() );
        int  response = wxMessageBox( tmp, "Potential issue", wxICON_WARNING | wxYES_NO );
        if (response == wxYES) {
            Preferences::setHome( s2 );
            //this is very important because it causes the write to the configuration file
            wxConfigBase*  pConfig = wxConfigBase::Get();
            assert( pConfig != NULL );
            pConfig->Flush();

            ::modifyEnvironment();
        }
    }
#else
    puts( "setting VIEWNIX_ENV" );
    setenv( "VIEWNIX_ENV", (const char *)Preferences::getHome().c_str(), 1 );
    puts( "setting PATH" );
    wxString  tmp = wxString::Format( "%s:%s", (const char *)Preferences::getHome().c_str(),
                                               getenv("PATH") );
    setenv( "PATH", (const char *)tmp.c_str(), 1 );
#endif
    printf( "VIEWNIX_ENV now set to %s \n", getenv("VIEWNIX_ENV") );
    printf( "PATH        now set to %s \n", getenv("PATH") );
}
//----------------------------------------------------------------------

