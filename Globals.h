/*
  Copyright 1993-2010 Medical Image Processing Group
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
#ifndef __Globals_h
#define __Globals_h

extern char*  button_zoomin15_xpm[];
extern char*  button_zoomout15_xpm[];

#define  defaultSashSize  10
#define  buttonWidth    90
#if defined (WIN32) || defined (_WIN32)
    #define  buttonHeight   22
#else
    #define  buttonHeight   28 //30
#endif
#define  controlsWidth  300
#define  sliderWidth    120
#define  ButtonOffset   10

//#define  CustomAppearance
#define  WIDTH   (wxSystemSettings::GetMetric(wxSYS_SCREEN_X)-100)
#define  HEIGHT  (wxSystemSettings::GetMetric(wxSYS_SCREEN_Y)-100)
//#define  WIDTH  1100
//#define  HEIGHT 600

#define  Yellow    255,255,167
#define  MenuBlue  93,92,127
#define  LtBlue    69,69,89
#define  DkBlue    37,51,62
#define  Selected  117,78,118
#define  White     242,200,188
#define  Red       255,0,0
#define  Green     0,255,0
#define  Magenta   0xbe,0x24,0xae

#define  BoxTop

#include  <vector>
class FileHistory;
class MainFrame;
class MontageFrame;
//typedef  std::vector< MontageFrame* >  Vector;
typedef  std::vector< wxFrame* >  Vector;
extern Vector            demonsInputList;
extern wxPrintData*      g_printData;        //global print data - to remember
                                             // settings during the session
extern wxPageSetupData*  g_pageSetupData;    //global page setup data
#ifdef __MACH__
  #define cWhereIncr 50
#else
  #define cWhereIncr 25
#endif

extern int               gWhere;             //position of new window
extern FileHistory       gInputFileHistory;  //global input file history
extern wxFont            gDefaultFont;       //default font

//use 1/gHistogramBucketFactor buckets for the histogram, i.e., divide each
// gray value by this factor before counting it
extern int               gHistogramBucketFactor;

template< typename T >
void histogram ( const T* const data, const int numberOfPixels,
    const int bucketFactor=1, const char* const title=NULL )
{
    double  sum = 0.0;
    T  min = data[0];
    T  max = data[0];
    int  i;
    for (i=0; i<numberOfPixels; i++) {
        if (data[i]<min)    min = data[i];
        if (data[i]>max)    max = data[i];
        sum += data[i];
    }
    const double  mean = sum / numberOfPixels;
    const T  actualMin = min;
    const T  actualMax = max;
    min /= bucketFactor;
    max /= bucketFactor;

    unsigned long*  h = (unsigned long*)malloc(
        (max-min+1)*sizeof(unsigned long) );
    assert( h!=NULL );
    for (i=0; i<max-min+1; i++)         h[i]=0;
    double  stddev = 0.0;
    for (i=0; i<numberOfPixels; i++) {
        ++h[ data[i]/bucketFactor - min ];
        stddev += (data[i]-mean)*(data[i]-mean);
    }
    stddev /= numberOfPixels-1;
    stddev = sqrt(stddev);

    #if defined (WIN32) || defined (_WIN32)
        char  tmpName[255]="cavass-plot-data.txt";
        unlink( tmpName );
    #else
        char  tmpName[255] = "cavass-XXXXXX";
        mkstemp(tmpName);
    #endif
    FILE*  tmpData = fopen( tmpName, "wb");
    assert( tmpData!=NULL );

    // cout << "histogram:" << endl;
    for (i=0; i<max-min+1; i++) {
        if (h[i]>0)
            // cout << "h[" << (i+min)*bucketFactor << "]=" << h[i] << endl;
        fprintf( tmpData, "%d %d \n", (int)((i+min)*bucketFactor), (int)h[i] );
    }

    fclose(tmpData);    tmpData=NULL;
    #if defined (WIN32) || defined (_WIN32)
        char  tmpName2[255]="cavass-plot-commands.txt";
        unlink( tmpName2 );
    #else
        char  tmpName2[255] = "cavass-XXXXXX";
        mkstemp(tmpName2);
    #endif
    FILE*  tmpData2 = fopen( tmpName2, "wb");
    assert( tmpData2!=NULL );

    if (title!=NULL) {
        fprintf( tmpData2, "set title \"%s, mean=%.2f, stddev=%.2f\"\n",
            title, mean, stddev );
    } else {
        fprintf( tmpData2, "set title \"mean=%.2f, stddev=%.2f\"\n",
            mean, stddev );
    }
    fprintf( tmpData2, "plot '%s' using 1:2 title \"# buckets=%d\", \\\n",
        tmpName, (max-min+1) );
    fprintf( tmpData2,
        "     '%s' using 1:2 smooth csplines title \"min=%d, max=%d\"\n",
        tmpName, actualMin, actualMax );
    fclose(tmpData2);    tmpData2=NULL;

    char  plotCommand[255];
    #if defined (WIN32) || defined (_WIN32)
        snprintf( plotCommand, sizeof plotCommand,
                  "\\gnuplot\\bin\\wgnuplot %s -", tmpName2 );
        wxLogMessage( "data: %s, commands: %s, command: %s", tmpName,
            tmpName2, plotCommand );
        system( plotCommand );
    #else
        snprintf( plotCommand, sizeof plotCommand, "gnuplot -persist %s",
                  tmpName2 );
        wxLogMessage( "data: %s, commands: %s, command: %s", tmpName,
            tmpName2, plotCommand );
        system( plotCommand );
        // unlink(tmpName);    unlink(tmpName2);
    #endif
    free(h);  h=NULL;
}

template< typename T, typename T2, typename TR >
void histogramWithTransform ( const T* const data, const int numberOfPixels,
    const int bucketFactor, const T2 actualMin, const T2 actualMax,
    const TR* const transform, const char* const title=NULL )
{
    const T2  min = actualMin / bucketFactor;
    const T2  max = actualMax / bucketFactor;

    unsigned long*  h = (unsigned long*)malloc(
        (max-min+1)*sizeof(unsigned long) );
    assert( h!=NULL );
    int  i;
    for (i=0; i<max-min+1; i++)         h[i]=0;
    for (i=0; i<numberOfPixels; i++)
        ++h[ transform[ data[i]-actualMin ]/bucketFactor - min ];
    #if defined (WIN32) || defined (_WIN32)
        char  tmpName[255]="cavass-plot-data.txt";
        unlink( tmpName );
    #else
        char  tmpName[255] = "cavass-XXXXXX";
        mkstemp(tmpName);
    #endif
    FILE*  tmpData = fopen( tmpName, "wb");
    assert( tmpData!=NULL );

    // cout << "histogram with transform:" << endl;
    for (i=0; i<max-min+1; i++) {
        if (h[i]>0)
            // cout << "h[" << (i+min)*bucketFactor << "]=" << h[i] << endl;
        fprintf( tmpData, "%d %d \n", (int)((i+min)*bucketFactor), (int)h[i] );
    }

    fclose(tmpData);    tmpData=NULL;
    #if defined (WIN32) || defined (_WIN32)
        char  tmpName2[255]="cavass-plot-commands.txt";
        unlink( tmpName2 );
    #else
        char  tmpName2[255] = "cavass-XXXXXX";
        mkstemp(tmpName2);
    #endif
    FILE*  tmpData2 = fopen( tmpName2, "wb");
    assert( tmpData2!=NULL );

    if (title!=NULL)    fprintf( tmpData2, "set title \"%s\"\n", title );
    fprintf( tmpData2, "plot '%s' using 1:2 title \"# buckets=%d\", \\\n",
        tmpName, (max-min+1) );
    fprintf( tmpData2,
        "     '%s' using 1:2 smooth csplines title \"min=%d, max=%d\"\n",
        tmpName, actualMin, actualMax );
    fclose(tmpData2);    tmpData2=NULL;

    char  plotCommand[255];
    #if defined (WIN32) || defined (_WIN32)
        snprintf( plotCommand, sizeof plotCommand,
                  "\\gnuplot\\bin\\wgnuplot %s -", tmpName2 );
        wxLogMessage( "data: %s, commands: %s, command: %s", tmpName,
            tmpName2, plotCommand );
        system( plotCommand );
    #else
        snprintf( plotCommand, sizeof plotCommand, "gnuplot -persist %s",
                  tmpName2 );
        system( plotCommand );
        unlink(tmpName);    unlink(tmpName2);
    #endif
    free(h);  h=NULL;
}

bool equals ( wxFileName f1, wxFileName f2 );

void setBackgroundColor ( wxWindow* w );
void setColor           ( wxWindow* w );
void setBoxColor        ( wxWindow* w );
void setSliderBoxColor  ( wxWindow* w );
void setDisabledColor   ( wxWindow* w );
void setZoomInColor     ( wxWindow* w );

extern int  gNextWindowID;
void addToAllWindowMenus ( wxString s );
void removeFromAllWindowMenus ( wxString s );
bool searchWindowTitles ( wxString s );
void copyWindowTitles ( wxFrame* whichFrame );
void changeAllWindowMenus ( wxString from, wxString to );
void raiseWIndow ( wxString s );

class CavassData;
unsigned char* toRGB ( CavassData& cd );
unsigned char* toRGBInterpolated ( CavassData& cd, double sx, double sy );

#endif
