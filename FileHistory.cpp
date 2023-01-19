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
#include  "cavass.h"

//#define  FileHistoryDebug  //define for debugging message

FileHistory::FileHistory ( void ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileHistory::AddFileToHistory ( const wxString& filename ) {
    int  n = GetNoHistoryFiles();
    assert( 0<=n && n<Preferences::FileCount );
    int index;
    for (index=0; index<Preferences::FileCount-1 && index<n; index++)
    {
        wxString entry(Preferences::getFile( index ));
        if (entry==filename || entry==filename+"/.")
            break;
    }
    while (index > 0)
    {
        wxString entry(Preferences::getFile( index-1 ));
        Preferences::setFile( index-1, "" );
        Preferences::setFile( index, entry );
        index--;
    }
    Preferences::setFile( 0, filename+"/." );

    wxString  path, name, ext;
    wxFileName::SplitPath( filename, &path, &name, &ext );
    ::wxSetWorkingDirectory( path );
    Preferences::setInputDirectory( path );
    #ifdef  FileHistoryDebug
        wxLogMessage( "FileHistory::AddFileToHistory( " + filename + " )" );
        dump();
    #endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
wxString FileHistory::GetHistoryFile ( int index ) const {
    wxString entry(Preferences::getFile( index ));
    if (entry.Matches("*/."))
    {
        entry.RemoveLast();
        entry.RemoveLast();
    }
    return entry;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FileHistory::IsSelected ( int index ) const {
    wxString entry(Preferences::getFile( index ));
    return entry.Matches("*/.");
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FileHistory::GetMaxFiles ( void ) const {
    return Preferences::FileCount;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FileHistory::GetNoHistoryFiles ( void ) const {
    #ifdef  FileHistoryDebug
        wxLogMessage( "FileHistory::GetNoHistoryFiles()" );
        dump();
    #endif
    for (int i=0; i<Preferences::FileCount; i++) {
        if (Preferences::getFile(i).length()==0)    return i;
    }
    return Preferences::FileCount-1;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileHistory::dump ( void ) const {
    for (int i=0; i<Preferences::FileCount; i++) {
        wxLogMessage( Preferences::getFile(i) );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileHistory::Load ( wxConfigBase& config ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileHistory::clear ( const wxString& filter ) {
    wxArrayString patterns;
    wxString rest=filter;
    while (rest.Len())
    {
        int bar=rest.Find('|');
        int semicolon=rest.Find(';');
        if (bar == wxNOT_FOUND)
            bar = semicolon;
        if (semicolon==wxNOT_FOUND)
            semicolon = bar;
        if (semicolon < bar)
            bar = semicolon;
        if (bar == wxNOT_FOUND)
            bar = rest.Len();
#undef Left
        patterns.Add(rest.Left(bar));
        #ifdef  FileHistoryDebug
            wxLogMessage( "patterns.Add(\""+rest.Left(bar)+"\")" );
        #endif
        rest = rest.Mid(bar+1);
    }
    for (int i=0; i<Preferences::FileCount; i++) {
        wxString entry(Preferences::getFile( i ));
        if (entry.Matches("*/."))
        {
            entry.RemoveLast();
            entry.RemoveLast();
            for (size_t j=0; j<patterns.Count(); j++)
                if (entry.Matches(patterns[j]))
                {
                    Preferences::setFile( i, entry );
                    #ifdef  FileHistoryDebug
                        wxLogMessage( entry + " unselected." );
                    #endif
                    break;
                }
        }
    }
    #ifdef  FileHistoryDebug
        wxLogMessage( "FileHistory::clear( \"" + filter + "\" )" );
        dump();
    #endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileHistory::Save ( wxConfigBase& config ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

