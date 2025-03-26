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
 * \file   Preferences.h
 * \brief  Definition and implementation of Preferences class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#pragma once
//#ifndef  __Preferences_h
//#define  __Preferences_h

/** \brief Definition and implementation of Preferences class.
 *
 *  This (singleton) class allows the caller to check and modify the
 *  status/state of user preferences.  This class allows preferences
 *  to persist via loading and storing preferences in a wxConfig object.
 *  Changes are automatically stored (to the file ~/.cavass.ini on Linux).
 */
class Preferences {
  public:
    enum {
        FileCount = 25,  ///< max number of open files
        HostCount = 50   ///< max number of hosts in the cluster
    };

    /** \brief this method returns a singleton instance of the Preferences
     *  class.
     *
     *  If the instance does not already exist, it will be created.
     */
    static Preferences* Instance ( ) {
        //if it already exists, then use it
        if (_instance != nullptr)    return _instance;

        //doesn't already exist. create it/set it up.
        _instance    = new Preferences;
        _preferences = wxConfigBase::Get();
        ::gInputFileHistory.Load( *_preferences );

        /** \todo retrieve values of additional preferences here. */

        _customAppearance = _preferences->Read( "customAppearance", _customAppearance );

        wxGetEnv( "VIEWNIX_ENV", &_home );

        //very important to set _home correctly
        _home = _preferences->Read( "home", _home );

        _hostNames = _preferences->Read( "hostNames", _hostNames );
        _hostProcessCounts = _preferences->Read( "hostProcessCounts", _hostProcessCounts );
        _inputDirectory = _preferences->Read( "inputDirectory", _inputDirectory );
        if (wxFileName::DirExists(_inputDirectory))
            ::wxSetWorkingDirectory( _inputDirectory );
        _lastFrame = _preferences->Read( "lastFrame", _lastFrame );
        _MPIDirectory = _preferences->Read( "MPIDirectory", _MPIDirectory );
        _outputDirectory = _preferences->Read( "outputDirectory", _outputDirectory );
        _parallelMode = _preferences->Read( "parallelMode", _parallelMode );
        _preferences->Write( "saveScreenFileName", "temp.tiff" );
        _saveScreenFileName = _preferences->Read( "saveScreenFileName",
                                                  _saveScreenFileName );
        _showLog          = _preferences->Read( "showLog",         _showLog );
        _showLog_x        = _preferences->Read( "showLog_x",       _showLog_x );
        _showLog_y        = _preferences->Read( "showLog_y",       _showLog_y );
        _showLog_w        = _preferences->Read( "showLog_w",       _showLog_w );
        _showLog_h        = _preferences->Read( "showLog_h",       _showLog_h );
        _showSaveScreen   = _preferences->Read( "showSaveScreen",  _showSaveScreen );
        _showToolTips     = _preferences->Read( "showToolTips",    _showToolTips );
        _singleFrameMode  = _preferences->Read( "singleFrameMode", _singleFrameMode );
        _dejaVuMode       = _preferences->Read( "dejaVuMode",      _dejaVuMode  );

        _stereoMode       = _preferences->Read( "stereoMode",      _stereoMode    );
        _stereoAngle      = _preferences->Read( "stereoAngle",     _stereoAngle   );

        _stereoLeftRed    = _preferences->Read( "stereoLeftRed",   _stereoLeftRed    );
        _stereoLeftGreen  = _preferences->Read( "stereoLeftGreen", _stereoLeftGreen  );
        _stereoLeftBlue   = _preferences->Read( "stereoLeftBlue",  _stereoLeftBlue   );

        _stereoRightRed   = _preferences->Read( "stereoRightRed",  _stereoRightRed    );
        _stereoRightGreen = _preferences->Read( "stereoRightGreen",_stereoRightGreen  );
        _stereoRightBlue  = _preferences->Read( "stereoRightBlue", _stereoRightBlue   );

        _stereoLeftOdd    = _preferences->Read( "stereoLeftOdd",   _stereoLeftOdd  );

        _useInputHistory  = _preferences->Read( "useInputHistory", _useInputHistory  );
        _customAppearance = _preferences->Read("customAppearance",_customAppearance );

        _fgRed   = _preferences->Read( "fgRed",   _fgRed   );
        _fgGreen = _preferences->Read( "fgGreen", _fgGreen );
        _fgBlue  = _preferences->Read( "fgBlue",  _fgBlue  );

        _bgRed   = _preferences->Read( "bgRed",   _bgRed   );
        _bgGreen = _preferences->Read( "bgGreen", _bgGreen );
        _bgBlue  = _preferences->Read( "bgBlue",  _bgBlue  );

        _CTLungCenter       = _preferences->Read( "CTLungCenter",       _CTLungCenter       );
        _CTLungWidth        = _preferences->Read( "CTLungWidth",        _CTLungWidth        );
        _CTSoftTissueCenter = _preferences->Read( "CTSoftTissueCenter", _CTSoftTissueCenter );
        _CTSoftTissueWidth  = _preferences->Read( "CTSoftTissueWidth",  _CTSoftTissueWidth  );
        _CTBoneCenter       = _preferences->Read( "CTBoneCenter",       _CTBoneCenter       );
        _CTBoneWidth        = _preferences->Read( "CTBoneWidth",        _CTBoneWidth        );
        _PETCenter          = _preferences->Read( "PETCenter",       _PETCenter       );
        _PETWidth           = _preferences->Read( "PETWidth",        _PETWidth        );

		_OverlayScale  = _preferences->Read( "OverlayScale",        _OverlayScale      );
		_IM0onIM0Red   = _preferences->Read( "IM0onIM0Red",   _IM0onIM0Red   );
		_IM0onIM0Green = _preferences->Read( "IM0onIM0Green", _IM0onIM0Green );
		_IM0onIM0Blue  = _preferences->Read( "IM0onIM0Blue",  _IM0onIM0Blue  );
		_BIMonIM0Red   = _preferences->Read( "BIMonIM0Red",   _BIMonIM0Red   );
		_BIMonIM0Green = _preferences->Read( "BIMonIM0Green", _BIMonIM0Green );
		_BIMonIM0Blue  = _preferences->Read( "BIMonIM0Blue",  _BIMonIM0Blue  );
		_BIMonBIMRed   = _preferences->Read( "BIMonBIMRed",   _BIMonBIMRed   );
		_BIMonBIMGreen = _preferences->Read( "BIMonBIMGreen", _BIMonBIMGreen );
		_BIMonBIMBlue  = _preferences->Read( "BIMonBIMBlue",  _BIMonBIMBlue  );

        for (int i=0; i<FileCount; i++) {
            wxString  tmp = wxString::Format( "file%d", i );
            _file[i] = _preferences->Read( tmp, _file[i] );
        }

        return _instance;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief this method deletes the instance. */
    static void DeleteInstance ( void ) {
        //_preferences->Flush();
        //delete wxConfigBase::Set( NULL );
        if (_instance!=0) {  delete _instance;  _instance = 0;  }
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for custom appearance preference.
     *  replaced getClassicColor().
     */
    static bool getCustomAppearance ( void ) {
        Preferences::Instance();
        if (_customAppearance)    return true;
        return false;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for red component of foreground color. */
    static int getFgRed ( void ) {
        Preferences::Instance();
        return _fgRed;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for green component of foreground color. */
    static int getFgGreen ( void ) {
        Preferences::Instance();
        return _fgGreen;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for blue component of foreground color. */
    static int getFgBlue ( void ) {
        Preferences::Instance();
        return _fgBlue;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for red component of background color. */
    static int getBgRed ( void ) {
        Preferences::Instance();
        return _bgRed;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for green component of background color. */
    static int getBgGreen ( void ) {
        Preferences::Instance();
        return _bgGreen;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for blue component of background color. */
    static int getBgBlue ( void ) {
        Preferences::Instance();
        return _bgBlue;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for lung window center. */
    static int getCTLungCenter ( void ) {
        Preferences::Instance();
        return _CTLungCenter;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for lung window width. */
    static int getCTLungWidth ( void ) {
        Preferences::Instance();
        return _CTLungWidth;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for soft tissue window center. */
    static int getCTSoftTissueCenter ( void ) {
        Preferences::Instance();
        return _CTSoftTissueCenter;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for soft tissue window width. */
    static int getCTSoftTissueWidth ( void ) {
        Preferences::Instance();
        return _CTSoftTissueWidth;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for bone window center. */
    static int getCTBoneCenter ( void ) {
        Preferences::Instance();
        return _CTBoneCenter;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for bone window width. */
    static int getCTBoneWidth ( void ) {
        Preferences::Instance();
        return _CTBoneWidth;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for bone window center. */
    static int getPETCenter ( void ) {
        Preferences::Instance();
        return _PETCenter;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for bone window width. */
    static int getPETWidth ( void ) {
        Preferences::Instance();
        return _PETWidth;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for overlay scale. */
    static double getOverlayScale ( void ) {
        Preferences::Instance();
		double val;
		_OverlayScale.ToDouble( &val );
        return val;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for red component of overlay color. */
    static int getIM0onIM0Red ( void ) {
        Preferences::Instance();
        return _IM0onIM0Red;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for green component of overlay color. */
    static int getIM0onIM0Green ( void ) {
        Preferences::Instance();
        return _IM0onIM0Green;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for blue component of overlay color. */
    static int getIM0onIM0Blue ( void ) {
        Preferences::Instance();
        return _IM0onIM0Blue;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for red component of overlay color. */
    static int getBIMonIM0Red ( void ) {
        Preferences::Instance();
        return _BIMonIM0Red;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for green component of overlay color. */
    static int getBIMonIM0Green ( void ) {
        Preferences::Instance();
        return _BIMonIM0Green;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for blue component of overlay color. */
    static int getBIMonIM0Blue ( void ) {
        Preferences::Instance();
        return _BIMonIM0Blue;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for red component of overlay color. */
    static int getBIMonBIMRed ( void ) {
        Preferences::Instance();
        return _BIMonBIMRed;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for green component of overlay color. */
    static int getBIMonBIMGreen ( void ) {
        Preferences::Instance();
        return _BIMonBIMGreen;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for blue component of overlay color. */
    static int getBIMonBIMBlue ( void ) {
        Preferences::Instance();
        return _BIMonBIMBlue;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static wxString getFile ( int which ) {
        assert( 0<=which && which<FileCount );
        Preferences::Instance();
        return _file[ which ];
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for CAVASS home directory preference. */
    static wxString getHome ( void ) {
        Preferences::Instance();
        return _home;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief get the name of the nth host in the cluster (for parallel
     *  processing).
     */
    static wxString getHostName ( int which ) {
        assert( 0 <= which && which < HostCount );
        Preferences::Instance();
        stringstream  stream( (const char *)_hostNames.c_str() );
        string  word;
        int  count = 0;
        while (getline(stream, word, ',')) {
            if (which == count)    return wxString(word);
            ++count;
        }
        return wxString("");
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief get the name of the nth host in the cluster (for parallel 
     *  processing).
     *  \returns the number of processes for a particular host (as a string
     *  because it's handier to have it as a string).
     */
    static wxString getHostProcessCount ( int which ) {
        assert( 0 <= which && which < HostCount );
        Preferences::Instance();
        stringstream  stream( (const char *)_hostProcessCounts.c_str() );
        string  word;
        int  count = 0;
        while (getline(stream, word, ',')) {
            if (which == count) {
                int  value = 0;
#ifndef  NDEBUG
                int  n =
#endif
                sscanf( (const char *)word.c_str(), "%d", &value );
                assert( n == 1 );
                return wxString( word );
            }
            ++count;
        }
//        assert( 0 );  //shouldn't get here
        return wxString( "0" );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for input directory preference. */
    static wxString getInputDirectory ( void ) {
        Preferences::Instance();
        return _inputDirectory;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for last frame accessed (viz., closed). */
    static wxString getLastFrame ( ) {
        Preferences::Instance();
        return _lastFrame;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for MPI directory preference. */
    static wxString getMPIDirectory ( void ) {
        Preferences::Instance();
        return _MPIDirectory;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for output directory preference. */
    static wxString getOutputDirectory ( void ) {
        Preferences::Instance();
        return _outputDirectory;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for parallel mode preference. */
    static bool getParallelMode ( void ) {
        Preferences::Instance();
        if (_parallelMode)    return true;
        return false;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for save screen file name preference. */
    static wxString getSaveScreenFileName ( void ) {
        Preferences::Instance();
        return _saveScreenFileName;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for show information log preference. */
    static bool getShowLog ( ) {
        Preferences::Instance();
        if (_showLog)    return true;
        return false;
    }

    /** \brief accessor for show information log preference. */
    static long getShowLog_x ( ) {
        Preferences::Instance();
        return _showLog_x;
    }

    /** \brief accessor for show information log preference. */
    static long getShowLog_y ( ) {
        Preferences::Instance();
        return _showLog_y;
    }

    /** \brief accessor for show information log preference. */
    static long getShowLog_w ( ) {
        Preferences::Instance();
        return _showLog_w;
    }

    /** \brief accessor for show information log preference. */
    static long getShowLog_h ( ) {
        Preferences::Instance();
        return _showLog_h;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for show save screen preference. */
    static bool getShowSaveScreen ( void ) {
        Preferences::Instance();
        if (_showSaveScreen)    return true;
        return false;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for show tooltips preference. */
    static bool getShowToolTips ( void ) {
        Preferences::Instance();
        if (_showToolTips)    return true;
        return false;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for single frame mode preference. */
    static bool getSingleFrameMode ( void ) {
        Preferences::Instance();
        if (_singleFrameMode)    return true;
        return false;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for use input history preference. */
    static bool getDejaVuMode ( void ) {
        Preferences::Instance();
        if (_dejaVuMode)    return true;
        return false;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for stereo mode preference. */
    static int getStereoMode ( void ) {
        Preferences::Instance();
        return _stereoMode;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for stereo angle preference. */
    static double getStereoAngle ( void ) {
        Preferences::Instance();
        double val;
        _stereoAngle.ToDouble( &val );
        return val;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for left red color preference for stereo anaglyph mode. */
    static int getStereoLeftRed ( void ) {
        Preferences::Instance();
        return _stereoLeftRed;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for left green color preference for stereo anaglyph mode. */
    static int getStereoLeftGreen ( void ) {
        Preferences::Instance();
        return _stereoLeftGreen;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for left blue color preference for stereo anaglyph mode. */
    static int getStereoLeftBlue ( void ) {
        Preferences::Instance();
        return _stereoLeftBlue;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for right red color preference for stereo anaglyph mode. */
    static int getStereoRightRed ( void ) {
        Preferences::Instance();
        return _stereoRightRed;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for right green color preference for stereo anaglyph mode. */
    static int getStereoRightGreen ( void ) {
        Preferences::Instance();
        return _stereoRightGreen;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for right blue color preference for stereo anaglyph mode. */
    static int getStereoRightBlue ( void ) {
        Preferences::Instance();
        return _stereoRightBlue;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for stereo interlaced rows (for interlaced mode).
     *  true if left is odd (and right is even).
     *  false if left is even (and right is odd).
     */
    static bool getStereoLeftOdd ( void ) {
        Preferences::Instance();
        if (_stereoLeftOdd)    return true;
        return false;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for use input history preference. */
    static bool getUseInputHistory ( void ) {
        Preferences::Instance();
        if (_useInputHistory)    return true;
        return false;
    }

    /** \todo add additional preference accessors here. */

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for custom appearance preference. */
    static void setCustomAppearance ( bool newValue ) {
        Preferences::Instance();
        _customAppearance = newValue;
        _preferences->Write( "customAppearance", _customAppearance );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setFgRed ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _fgRed = newValue;
        _preferences->Write( "fgRed", _fgRed );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setFgGreen ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _fgGreen = newValue;
        _preferences->Write( "fgGreen", _fgGreen );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setFgBlue ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _fgBlue = newValue;
        _preferences->Write( "fgBlue", _fgBlue );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setBgRed ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _bgRed = newValue;
        _preferences->Write( "bgRed", _bgRed );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setBgGreen ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _bgGreen = newValue;
        _preferences->Write( "bgGreen", _bgGreen );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setBgBlue ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _bgBlue = newValue;
        _preferences->Write( "bgBlue", _bgBlue );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setCTLungCenter ( int newValue ) {
        Preferences::Instance();
        _CTLungCenter = newValue;
        _preferences->Write( "CTLungCenter", _CTLungCenter );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setCTLungWidth ( int newValue ) {
        Preferences::Instance();
        _CTLungWidth = newValue;
        _preferences->Write( "CTLungWidth", _CTLungWidth );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setCTSoftTissueCenter ( int newValue ) {
        Preferences::Instance();
        _CTSoftTissueCenter = newValue;
        _preferences->Write( "CTSoftTissueCenter", _CTSoftTissueCenter );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setCTSoftTissueWidth ( int newValue ) {
        Preferences::Instance();
        _CTSoftTissueWidth = newValue;
        _preferences->Write( "CTSoftTissueWidth", _CTSoftTissueWidth );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setCTBoneCenter ( int newValue ) {
        Preferences::Instance();
        _CTBoneCenter = newValue;
        _preferences->Write( "CTBoneCenter", _CTBoneCenter );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setCTBoneWidth ( int newValue ) {
        Preferences::Instance();
        _CTBoneWidth = newValue;
        _preferences->Write( "CTBoneWidth", _CTBoneWidth );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setPETCenter ( int newValue ) {
        Preferences::Instance();
        _PETCenter = newValue;
        _preferences->Write( "PETCenter", _PETCenter );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setPETWidth ( int newValue ) {
        Preferences::Instance();
        _PETWidth = newValue;
        _preferences->Write( "PETWidth", _PETWidth );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setOverlayScale ( double newValue ) {
        Preferences::Instance();
        _OverlayScale = wxString::Format("%.2f", newValue);
        _preferences->Write( "OverlayScale", _OverlayScale );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setIM0onIM0Red ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _IM0onIM0Red = newValue;
        _preferences->Write( "IM0onIM0Red", _IM0onIM0Red );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setIM0onIM0Green ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _IM0onIM0Green = newValue;
        _preferences->Write( "IM0onIM0Green", _IM0onIM0Green );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setIM0onIM0Blue ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _IM0onIM0Blue = newValue;
        _preferences->Write( "IM0onIM0Blue", _IM0onIM0Blue );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setBIMonIM0Red ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _BIMonIM0Red = newValue;
        _preferences->Write( "BIMonIM0Red", _BIMonIM0Red );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setBIMonIM0Green ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _BIMonIM0Green = newValue;
        _preferences->Write( "BIMonIM0Green", _BIMonIM0Green );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setBIMonIM0Blue ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _BIMonIM0Blue = newValue;
        _preferences->Write( "BIMonIM0Blue", _BIMonIM0Blue );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setBIMonBIMRed ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _BIMonBIMRed = newValue;
        _preferences->Write( "BIMonBIMRed", _BIMonBIMRed );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setBIMonBIMGreen ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _BIMonBIMGreen = newValue;
        _preferences->Write( "BIMonBIMGreen", _BIMonBIMGreen );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static void setBIMonBIMBlue ( int newValue ) {
        Preferences::Instance();
        if (newValue<0)    newValue=0;
        if (newValue>255)  newValue=255;
        _BIMonBIMBlue = newValue;
        _preferences->Write( "BIMonBIMBlue", _BIMonBIMBlue );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for input file preference.
     *  \param which is the file preference number.
     *  \param newValue is the value for the particular file preference number.
     */
    static void setFile ( int which, wxString newValue ) {
        assert( 0<=which && which<FileCount );
        Preferences::Instance();
        //make sure that a lower entry doesn't already exist with the same file name
        if (newValue.length()>0) {
            for (int i=0; i<which; i++) {
                if (_file[i].Cmp(newValue)==0) {
                    Preferences::DeleteInstance();
                    return;  //already exists
                }
            }
        }

        wxString  tmp = wxString::Format( "file%d", which );
        _preferences->Write( tmp, newValue );
        _file[which] = newValue;
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for CAVASS home directory preference. */
    static void setHome ( wxString newValue ) {
        Preferences::Instance();
        _home = newValue;
        _preferences->Write( "home", _home );
        Preferences::DeleteInstance();
    }
    static void clearHostNamesAndProcessCounts ( void ) {
        Preferences::Instance();
        _hostNames = "";
        _hostProcessCounts = "";

        _preferences->Write( "hostNames",         _hostNames );
        _preferences->Write( "hostProcessCounts", _hostProcessCounts );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the name of the nth host in the cluster (for parallel
     *  processing) and the number of processes that run on this host.
     */
    static void setHostNameAndProcessCount ( int which, wxString host,
                                             wxString processCount )
    {
        assert( 0 <= which && which < HostCount );
        Preferences::Instance();
        stringstream  oldh( (const char *)_hostNames.c_str() );
        stringstream  oldc( (const char *)_hostProcessCounts.c_str() );
        string  newh = "", newc = "";
        for (int i=0; i<HostCount; i++) {
            bool  hok = false, cok = false;
            string  h = "", c = "";
            if (getline(oldh, h, ','))    hok = true;
            if (getline(oldc, c, ','))    cok = true;
            if (which == i) {
                h = host;
                c = processCount;
            }
            newh = newh + h + ',';
            newc = newc + c + ',';
        }
        _hostNames = newh;
        _hostProcessCounts = newc;
        _preferences->Write( "hostNames",         _hostNames );
        _preferences->Write( "hostProcessCounts", _hostProcessCounts );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for input directory preference. */
    static void setInputDirectory ( wxString newValue ) {
        Preferences::Instance();
        ::wxSetWorkingDirectory( newValue );
        _inputDirectory = newValue;
        _preferences->Write( "inputDirectory", newValue );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for last frame accessed (viz., closed). */
    static void setLastFrame ( wxString newValue ) {
        Preferences::Instance();
        _lastFrame = newValue;
        _preferences->Write( "lastFrame", _lastFrame );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for mpi directory preference. */
    static void setMPIDirectory ( wxString newValue ) {
        Preferences::Instance();
        _MPIDirectory = newValue;
        _preferences->Write( "MPIDirectory", _MPIDirectory );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for output directory preference. */
    static void setOutputDirectory ( wxString newValue ) {
        Preferences::Instance();
        _outputDirectory = newValue;
        _preferences->Write( "outputDirectory", _outputDirectory );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for parallel mode preference. */
    static void setParallelMode ( bool newValue ) {
        Preferences::Instance();
        if (newValue)    _parallelMode = true;
        else             _parallelMode = false;
        _preferences->Write( "parallelMode", _parallelMode );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for save screen filename preference. */
    static void setSaveScreenFileName ( wxString newValue ) {
        Preferences::Instance();
        _saveScreenFileName = newValue;
        _preferences->Write( "saveScreenFileName", _saveScreenFileName );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for show information log preference. */
    static void setShowLog ( bool newValue ) {
        Preferences::Instance();
        #if defined(wxUSE_TOOLTIPS) && !defined(__WXX11__)
            wxToolTip::Enable( newValue );
        #endif
        if (newValue)    _showLog = true;
        else             _showLog = false;
        _preferences->Write( "showLog", _showLog );
        Preferences::DeleteInstance();
    }

    static void setShowLog_x ( long newValue ) {
        Preferences::Instance();
        _preferences->Write( "showLog_x", newValue );
        Preferences::DeleteInstance();
    }
    static void setShowLog_y ( long newValue ) {
        Preferences::Instance();
        _preferences->Write( "showLog_y", newValue );
        Preferences::DeleteInstance();
    }

    static void setShowLog_w ( long newValue ) {
        Preferences::Instance();
        _preferences->Write( "showLog_w", newValue );
        Preferences::DeleteInstance();
    }
    static void setShowLog_h ( long newValue ) {
        Preferences::Instance();
        _preferences->Write( "showLog_h", newValue );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for show save screen preference. */
    static void setShowSaveScreen ( bool newValue ) {
        Preferences::Instance();
        #if defined(wxUSE_TOOLTIPS) && !defined(__WXX11__)
            wxToolTip::Enable( newValue );
        #endif
        if (newValue)    _showSaveScreen = true;
        else             _showSaveScreen = false;
        _preferences->Write( "showSaveScreen", _showSaveScreen );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for show tooltips preference. */
    static void setShowToolTips ( bool newValue ) {
        Preferences::Instance();
        #if defined(wxUSE_TOOLTIPS) && !defined(__WXX11__)
            wxToolTip::Enable( newValue );
        #endif
        if (newValue)    _showToolTips = true;
        else             _showToolTips = false;
        _preferences->Write( "showToolTips", _showToolTips );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for single frame mode preference. */
    static void setSingleFrameMode ( bool newValue ) {
        Preferences::Instance();
        if (newValue)    _singleFrameMode = true;
        else             _singleFrameMode = false;
        _preferences->Write( "singleFrameMode", _singleFrameMode );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for use input history preference. */
    static void setDejaVuMode ( bool newValue ) {
        Preferences::Instance();
        if (newValue)    _dejaVuMode = true;
        else             _dejaVuMode = false;
        _preferences->Write( "dejaVuMode", _dejaVuMode );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for stereo mode preference. */
    static void setStereoMode ( int newValue ) {
        Preferences::Instance();
        assert( newValue==StereoModeOff || newValue==StereoModeAnaglyph || newValue==StereoModeInterlaced );
        _stereoMode = newValue;
        _preferences->Write( "stereoMode", _stereoMode );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for stereo angle preference. */
    static void setStereoAngle ( double newValue ) {
        Preferences::Instance();
        _stereoAngle = wxString::Format( "%.2f", newValue );
        _preferences->Write( "stereoAngle", _stereoAngle );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for left red color preference for stereo anaglyph mode. */
    static void setStereoLeftRed ( int newValue ) {
        Preferences::Instance();
        _stereoLeftRed = newValue;
        _preferences->Write( "stereoLeftRed", _stereoLeftRed );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for left green color preference for stereo anaglyph mode. */
    static void setStereoLeftGreen ( int newValue ) {
        Preferences::Instance();
        _stereoLeftGreen = newValue;
        _preferences->Write( "stereoLeftGreen", _stereoLeftGreen );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for left blue color preference for stereo anaglyph mode. */
    static void setStereoLeftBlue ( int newValue ) {
        Preferences::Instance();
        _stereoLeftBlue = newValue;
        _preferences->Write( "stereoLeftBlue", _stereoLeftBlue );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for right red color preference for stereo anaglyph mode. */
    static void setStereoRightRed ( int newValue ) {
        Preferences::Instance();
        _stereoRightRed = newValue;
        _preferences->Write( "stereoRightRed", _stereoRightRed );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for right green color preference for stereo anaglyph mode. */
    static void setStereoRightGreen ( int newValue ) {
        Preferences::Instance();
        _stereoRightGreen = newValue;
        _preferences->Write( "stereoRightGreen", _stereoRightGreen );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief accessor for right blue color preference for stereo anaglyph mode. */
    static void setStereoRightBlue ( int newValue ) {
        Preferences::Instance();
        _stereoRightBlue = newValue;
        _preferences->Write( "stereoRightBlue", _stereoRightBlue );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for stereo interlaced rows (for interlaced mode).
     *  true if left is odd (and right is even).
     *  false if left is even (and right is odd).
     */
    static void setStereoLeftOdd ( bool newValue ) {
        Preferences::Instance();
        _stereoLeftOdd = newValue;
        _preferences->Write( "stereoLeftOdd", _stereoLeftOdd );
        Preferences::DeleteInstance();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief mutator for use input history preference. */
    static void setUseInputHistory ( bool newValue ) {
        Preferences::Instance();
        if (newValue)    _useInputHistory = true;
        else             _useInputHistory = false;
        _preferences->Write( "useInputHistory", _useInputHistory );
        Preferences::DeleteInstance();
    }

    /** \todo add additional preference mutators here. */

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static bool getPersistence (  const string& group, const string& key, int& currentValue, int defaultValue );
//    static bool getPersistence (  const string& group, const char* key, int& currentValue, int defaultValue );
//    static bool getPersistence (  const string& group, const char* key, bool& currentValue, bool defaultValue );
    static bool getPersistence (  const string& group, const string& key, string& currentValue, string& defaultValue );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  private:
    /** \brief ctor - do not use! this is a singleton. */
    Preferences ( void ) { }

    static Preferences*   _instance;       ///< single preferences object
    static wxConfigBase*  _preferences;    ///< single wxConfig object (to make preferences persists).

    /** \todo define/add additional user preferences here. */
    static long      _customAppearance; ///< 0=default; 1=custom appearance on
    static wxString  _home;             ///< CAVASS home directory
    static wxString  _inputDirectory;   ///< input directory
    static wxString  _lastFrame;        ///< last frame accessed (viz., closed)
    static wxString  _MPIDirectory;     ///< mpi directory
    static wxString  _outputDirectory;  ///< output directory
    static long      _parallelMode;     ///< 0=default; 1=parallel mode on
    static wxString  _saveScreenFileName;  ///< file name of file to which screens are saved
    static long      _showLog;          ///< 0=off; 1=show information log enabled
    static long      _showLog_x;        ///< log window position, x
    static long      _showLog_y;        ///< log window position, y
    static long      _showLog_w;        ///< log window width
    static long      _showLog_h;        ///< log window height
    static long      _showSaveScreen;   ///< 0=off; 1=show save screen enabled
    static long      _showToolTips;     ///< 0=off; 1=tooltips enabled
    static long      _singleFrameMode;  ///< 0=multi frame mode; 1=single frame mode
    static long      _dejaVuMode;       ///< 0=off; 1=on
public:
    enum {           StereoModeOff, StereoModeAnaglyph, StereoModeInterlaced };  ///< stereo modes
private:
    static long      _stereoMode;       ///< stereo off, anaglyph, or interlaced
    static wxString  _stereoAngle;      ///< stereo angle value (in y direction) in degrees
    static int       _stereoLeftRed,  _stereoLeftGreen,  _stereoLeftBlue;   ///< stereo anaglyph left rgb color
    static int       _stereoRightRed, _stereoRightGreen, _stereoRightBlue;  ///< stereo anaglyph right rgb color
    static int       _stereoLeftOdd;    ///< stereo interlaced rows 1=left-odd-right-even; 0=left-even-right-odd

    static long      _useInputHistory;  ///< 0=don't use; 1=use input (from) history

    static int       _fgRed, _fgGreen, _fgBlue;  ///< rgb foreground color
    static int       _bgRed, _bgGreen, _bgBlue;  ///< rgb background color

    static int       _CTLungCenter, _CTLungWidth;             ///< gray window
    static int       _CTSoftTissueCenter, _CTSoftTissueWidth; ///< gray window
    static int       _CTBoneCenter, _CTBoneWidth;             ///< gray window
    static int       _PETCenter, _PETWidth;             ///< gray window

	static wxString  _OverlayScale;             ///< image scale
	static int       _IM0onIM0Red, _IM0onIM0Green, _IM0onIM0Blue,
                     _BIMonIM0Red, _BIMonIM0Green, _BIMonIM0Blue,
                     _BIMonBIMRed, _BIMonBIMGreen, _BIMonBIMBlue; ///< overlay

    static wxString  _file[ Preferences::FileCount ];

    //each string below is delimited by a comma
    static wxString  _hostNames;          ///< string of cluster host/system names stored in registry
    static wxString  _hostProcessCounts;  ///< string of number of processes for a particular host
};
//----------------------------------------------------------------------
void modifyEnvironment ( char* cavassExe = NULL );

//#endif
