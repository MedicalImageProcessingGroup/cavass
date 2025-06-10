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
//#include  <unistd.h>

Preferences*   Preferences::_instance    = nullptr;
wxConfigBase*  Preferences::_preferences = nullptr;

long           Preferences::_customAppearance = 1;
//background color (dark blue):
int            Preferences::_bgBlue  = 89;
int            Preferences::_bgGreen = 69;
int            Preferences::_bgRed   = 69;
//foreground color (yellow):
int            Preferences::_fgBlue  = 167;
int            Preferences::_fgGreen = 255;
int            Preferences::_fgRed   = 255;

wxString       Preferences::_file[ Preferences::FileCount ];
wxString       Preferences::_hostNames( "" );
wxString       Preferences::_hostProcessCounts( "" );
wxString       Preferences::_inputDirectory(  wxGetUserHome() );
wxString       Preferences::_lastFrame( "" );
wxString       Preferences::_outputDirectory( wxGetUserHome() );
long           Preferences::_parallelMode    = 0;
wxString       Preferences::_saveScreenFileName( "temp.tiff" );
long           Preferences::_showLog         = 0;
long           Preferences::_showLog_x       = 100;
long           Preferences::_showLog_y       = 100;
long           Preferences::_showLog_w       = 600;
long           Preferences::_showLog_h       = 400;
long           Preferences::_showSaveScreen  = 0;
long           Preferences::_showToolTips    = 0;
long           Preferences::_singleFrameMode = 1;
long           Preferences::_dejaVuMode      = 1;

long           Preferences::_stereoMode      = StereoModeOff;
wxString       Preferences::_stereoAngle     = "4.0";
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

wxString       Preferences::_OverlayScale  = "1.5";
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
    //wxString   Preferences::_home( "c:/cavass-build/debug" );
    wxString   Preferences::_MPIDirectory( "c:/Program Files/MPICH2/bin" );
#else
    //wxString   Preferences::_home( "." );
    wxString   Preferences::_MPIDirectory( "" );
#endif
    wxString   Preferences::_home( wxFileName::GetCwd() );
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
    static char*  argv0 = nullptr;

    if (cavassExe != nullptr) {
	    if (argv0)
			free(argv0);
		argv0 = (char *)malloc(strlen(cavassExe)+1);
		strcpy(argv0, cavassExe);
    }
    //if VIEWNIX_ENV is not set, set it to something reasonable (viz., the current wd)
#if 0
#if defined (WIN32) || defined (_WIN32)
    if (getenv("VIEWNIX_ENV") == nullptr) {
        char buff[1024];
        getcwd( buff, sizeof buff );
        wxString s( "\n\nVIEWNIX_ENV is not set!\n\nSetting it to " );
        s += buff;
        s += ".\n";
        wxMessageBox( s );
        setenv( "VIEWNIX_ENV", buff, 1 );
    } else {
        printf("VIEWNIX_ENV currently set to %s \n", getenv("VIEWNIX_ENV"));
    }
#else
    /** \todo need to do similar to above for windoze */
#endif
#else
    //below should work on both windoze and linux/mac
    if (getenv("VIEWNIX_ENV") == nullptr) {
        char buff[1024];
        getcwd( buff, sizeof buff );
        wxString s( "\n\nVIEWNIX_ENV is not set!\n\nSetting it to " );
        s += buff;
        s += ".\n";
        //wxMessageBox( s );
        cerr << s << endl;
        //setenv( "VIEWNIX_ENV", buff, 1 );
        wxSetEnv( "VIEWNIX_ENV", buff );
    } else {
        printf("VIEWNIX_ENV currently set to %s \n", getenv("VIEWNIX_ENV"));
    }
#endif

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
/**
 * this function allows one to retrieve a saved/persistent value without
 * having to actually create the widget.
 * example of retrieving level slider value from gray map controls:
 * <pre>
 *     int value;
 *     bool ok = Preferences::getPersistence( GrayMapControls::levelGroup, "value", value, -1 );
 * </pre>
 * example contents of ~/.cavass.ini for graymap controls:
 * <pre>
 * [Persistent_Options]
 * [Persistent_Options/wxSlider]
 * [Persistent_Options/wxSlider/graymap.level]
 * value=550
 * min=0
 * max=4095
 * [Persistent_Options/wxSlider/graymap.width]
 * value=1730
 * min=0
 * max=4095
 * </pre>
 * @param group is a string identifying the specific control
 * @param key is a string identifying the particular control value (there may be more than one) to retrieve
 * @param currentValue value will be set to the persistent/saved value
 * @param defaultValue is the value that currentValue will be set to whenever any problem occurs
 * @return true on success (currentValue will be set to the retrieved value); false otherwise (and currentValue will be set to the default value)
 */
bool Preferences::getPersistence ( const string& group, const string& key,
                                   int& currentValue, int defaultValue )
{
    Preferences::Instance();
    if (_preferences == nullptr) {
        currentValue = defaultValue;
        return false;
    }
    if (!_preferences->HasGroup(group)) {
        currentValue = defaultValue;
        return false;
    }

    auto oldPath = _preferences->GetPath();  //save (restore later)
    _preferences->SetPath( group );
    auto n = _preferences->GetNumberOfEntries();
    if (n < 1) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }
    if (!_preferences->HasEntry( key )) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }
    auto s = _preferences->Read( key );
    if (!s.IsNumber()) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }
    int tmp;
    if (!s.ToInt( &tmp )) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }

    currentValue = tmp;
    _preferences->SetPath( oldPath );  //restore
    return true;
}
#if 0
bool Preferences::getPersistence ( const string& group, const char* key,
                                   int& currentValue, int defaultValue )
{
    Preferences::Instance();
    if (_preferences == nullptr) {
        currentValue = defaultValue;
        return false;
    }
    if (!_preferences->HasGroup(group)) {
        currentValue = defaultValue;
        return false;
    }

    auto oldPath = _preferences->GetPath();  //save (restore later)
    _preferences->SetPath( group );
    auto n = _preferences->GetNumberOfEntries();
    if (n < 1) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }
    if (!_preferences->HasEntry( key )) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }
    auto s = _preferences->Read( key );
    if (!s.IsNumber()) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }
    int tmp;
    if (!s.ToInt( &tmp )) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }

    currentValue = tmp;
    _preferences->SetPath( oldPath );  //restore
    return true;
}

bool Preferences::getPersistence ( const string& group, const char* key,
                                   bool& currentValue, bool defaultValue )
{
    Preferences::Instance();
    if (_preferences == nullptr) {
        currentValue = defaultValue;
        return false;
    }
    if (!_preferences->HasGroup(group)) {
        currentValue = defaultValue;
        return false;
    }

    auto oldPath = _preferences->GetPath();  //save (restore later)
    _preferences->SetPath( group );
    auto n = _preferences->GetNumberOfEntries();
    if (n < 1) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }
    if (!_preferences->HasEntry( key )) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }
    auto s = _preferences->Read( key );
    if (!s.IsNumber()) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }
    int tmp;
    if (!s.ToInt( &tmp )) {
        currentValue = defaultValue;
        _preferences->SetPath( oldPath );  //restore
        return false;
    }

    currentValue = tmp;
    _preferences->SetPath( oldPath );  //restore
    return true;
}
#endif

bool Preferences::getPersistence (  const string& group, const string& key,
                                    string& currentValue, string& defaultValue )
{
    /** \todo */
    assert( false );
    cout << "todo \n";
    return false;
}
//----------------------------------------------------------------------

