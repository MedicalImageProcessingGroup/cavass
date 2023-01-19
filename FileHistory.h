/*
  Copyright 1993-2014 Medical Image Processing Group
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
//======================================================================
#ifndef __FileHistory_h
#define __FileHistory_h

/** \todo should be make into a singleton
 */
class FileHistory {
public:
    FileHistory ( void );

    void      AddFileToHistory ( const wxString& filename );
    void      clear ( const wxString& filter );
    wxString  GetHistoryFile ( int index ) const;
    bool      IsSelected ( int index ) const;
    int       GetMaxFiles ( void ) const;
    int       GetNoHistoryFiles ( void ) const;
    void      Load ( wxConfigBase& config );
    void      Save ( wxConfigBase& config );
    void      dump ( void ) const;
};

#endif
