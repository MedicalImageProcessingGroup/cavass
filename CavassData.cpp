/*
  Copyright 1993-2015 Medical Image Processing Group
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

#include  "cavass.h"
#include  "CavassData.h"

void *CavassData::getSlice ( const int which ) {
        if (which<0 || which>=m_zSize || m_data==0) {
            return 0;
        }
        assert( mEntireVolumeIsLoaded );
        //determine the start addr of the slice
        switch (m_size) {
            case 1:
                {
                    unsigned char*  ucptr = (unsigned char*)m_data;
                    return &ucptr[ m_zsub[which] ];
                }
                break;
            case 2:
                {
                    unsigned short*  usptr = (unsigned short*)m_data;
                    return &usptr[ m_zsub[which] ];
                }
                break;
            case 3:
            case 4:
                {
                    unsigned int*  uiptr = (unsigned int*)m_data;
                    return &uiptr[ m_zsub[which] ];
                }
                break;
            default:
                assert( 0 );
                break;
        }
        return 0;
}

void CavassData::loadData ( char* name,
        const int    xSize,    const int    ySize,    const int    zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh,
        const bool vh_initialized )
    {
        if (vh!=NULL && vh_initialized) {
            m_vh = *vh;
            m_vh_initialized = true;
        }
        
        if (name==NULL || strlen(name)==0)  name= (char *)"no name";
        m_fname = (char*)malloc(strlen(name)+1);
        assert( m_fname != NULL );
        strcpy(m_fname, name);
        if (mLogLevel >= 1)
			cout << "loading " << name << endl;
        if (mLogLevel >= 2)
		{
			wxLogMessage("loading %s", name);
		}
        
        m_xSize = xSize;
        m_ySize = ySize;
        m_zSize = zSize;
        m_size  = sizeof(*data);
        m_bytesPerSlice = m_size * m_xSize * m_ySize;
        m_data = (void*)malloc( m_bytesPerSlice * m_zSize );
        if (m_data==NULL) {
            cerr << "Out of memory while loading " << name << endl;
            return;
        }
        //make our own copy of the data
        unsigned char*  t     = (unsigned char*)data;
        unsigned char*  ucPtr = (unsigned char*)m_data;
        for (int i=0; i<m_bytesPerSlice*m_zSize; i++) {
            ucPtr[i] = t[i];
        }
        
        //determine the global min & max
        if (m_size==1) {
            unsigned char*  ucData = (unsigned char*)m_data;
            m_min = m_max = ucData[0];
            for (int i=1; i<m_xSize*m_ySize*m_zSize; i++) {
                if (ucData[i]<m_min)    m_min=ucData[i];
                if (ucData[i]>m_max)    m_max=ucData[i];
            }
        } else if (m_size==2) {
            unsigned short*  sData = (unsigned short*)m_data;
            m_min = m_max = sData[0];
            for (int i=1; i<m_xSize*m_ySize*m_zSize; i++) {
                if (sData[i]<m_min)    m_min=sData[i];
                if (sData[i]>m_max)    m_max=sData[i];
            }
        } else if (m_size==4) {
            int*  iData = (int*)m_data;
            m_min = m_max = iData[0];
            for (int i=1; i<m_xSize*m_ySize*m_zSize; i++) {
                if (iData[i]<m_min)    m_min=iData[i];
                if (iData[i]>m_max)    m_max=iData[i];
            }
        } else {
            assert(0);
        }
		if (mLogLevel >= 1)
	        cout << "min=" << m_min << ", max=" << m_max << endl;
        if (mLogLevel >= 2)
		{
			wxLogMessage( "min=%d, max=%d", m_min, m_max );
        }
		m_lut = (unsigned char*)malloc( (m_max-m_min+1)*sizeof(unsigned char) );
        assert( m_lut != NULL );
        if (m_min==0 && m_max<=1)
        {
            m_center = 0;
            m_width  = 1;
        }
        else
        {
            m_center = m_min + (m_max-m_min+1)/2;
            m_width  = m_max-m_min+1;
        }
        initLUT();
        mEntireVolumeIsLoaded = 1;
}

int CavassData::loadFile ( const char* const fn, const bool loadHeaderOnly, bool grayOnly ) {
        if (fn==NULL || strlen(fn)==0)    return ERR_FILENAME;
        m_fname = (char*)malloc(strlen(fn)+1);
        assert( m_fname != NULL );
        strcpy(m_fname, fn);
        if (mLogLevel >= 1)
			cout << "reading " << fn << endl;
        if (mLogLevel >= 2)
		{
			wxLogMessage("reading %s", fn);
		}
        wxFileName wxfn(fn);
        if (endsWith(fn,".dcm") || endsWith(fn,".dicom") || !wxfn.HasExt()) {
            //handle a dicom file
            loadDicomFile();
        } else if (endsWith(fn,".jpg") || endsWith(fn,".jpeg") ||
                   endsWith(fn,".gif") || endsWith(fn,".png")) {
            loadImageFile( grayOnly );
        } else if (endsWith(fn,".tif") || endsWith(fn,".tiff")) {
#ifdef WIN32_TEST
            loadImageFile( grayOnly );
#else
            loadImageFileTIFF( grayOnly );
#endif
        } else if (endsWith(fn,".im0") || endsWith(fn,".bim") || endsWith(fn,".mv0")) {
            //handle a cavass/3dviewnix file
            int  err = loadCavassFile( loadHeaderOnly );
            if (err>0)             return ERR_LOADCAVASSFILE;
            if (loadHeaderOnly)    return 0;
        } else if (endsWith(fn,".vtk")) {
            loadVtkFile( loadHeaderOnly );
            if (loadHeaderOnly)    return 0;
        } else {
            if (mLogLevel >= 1)
			  cerr << "    Unrecognized file extension: " << fn << "." << endl;
			if (mLogLevel >= 2)
			{
				wxLogMessage( "    Unrecognized file extension: %s.", fn );
			}
        }
        
        determineGlobalMinAndMax();
        m_lut = (unsigned char*)malloc( (m_max-m_min+1)*sizeof(unsigned char) );
        assert( m_lut != NULL );
        if (m_min==0 && m_max<=1)
        {
            m_center = 0;
            m_width  = 1;
        }
        else
        {
            m_center = m_min + (m_max-m_min+1)/2;
            m_width  = m_max-m_min+1;
        }
        initLUT();

        return 0;
}

void CavassData::determineGlobalMinAndMax ( void ) {
        //determine the global min & max
        if (m_size==1) {
            unsigned char*  ucData = (unsigned char*)m_data;
            m_min = m_max = ucData[0];
            for (int i=1; i<m_xSize*m_ySize*m_zSize*mSamplesPerPixel; i++) {
                if (ucData[i]<m_min)    m_min=ucData[i];
                if (ucData[i]>m_max)    m_max=ucData[i];
            }
        } else if (m_size==2) {
            unsigned short*  sData = (unsigned short*)m_data;
            int  i;
            //cout << "loadFile: making short data 12 bits." << endl;
            //sData[0] &= 0xfff;
            m_min = m_max = sData[0];
            for (i=1; i<m_xSize*m_ySize*m_zSize*mSamplesPerPixel; i++) {
                //sData[i] &= 0xfff;
                if (sData[i]<m_min)    m_min=sData[i];
                if (sData[i]>m_max)    m_max=sData[i];
            }
        } else if (m_size==4) {
            int*  iData = (int*)m_data;
            m_min = m_max = iData[0];
            for (int i=1; i<m_xSize*m_ySize*m_zSize*mSamplesPerPixel; i++) {
                if (iData[i]<m_min)    m_min=iData[i];
                if (iData[i]>m_max)    m_max=iData[i];
            }
        }
		if (mLogLevel >= 1)
	        cout << "min=" << m_min << ", max=" << m_max << endl;
        if (mLogLevel >= 2)
		{
			wxLogMessage( "min=%d, max=%d", m_min, m_max );
		}
}

void CavassData::initSubs ( void ) {
        assert( m_ysub==NULL );
        m_ysub = (unsigned long*)malloc( m_ySize * sizeof(unsigned long) );
        assert( m_ysub!=NULL );
        for (int y=0; y<m_ySize; y++) {
            m_ysub[y] = y * m_xSize * mSamplesPerPixel;
        }

        assert( m_zsub==NULL );
        m_zsub = (unsigned long*)malloc( m_zSize * sizeof(unsigned long) );
        assert( m_zsub!=NULL );
        for (int z=0; z<m_zSize; z++) {
            m_zsub[z] = z * m_ySize * m_xSize * mSamplesPerPixel;
        }
}

void CavassData::initLUT ( void )
{
    //    if (m_data==NULL)    return;   // m_data == NULL when only load slice image  //xinjian
        
        const int  left  = m_center - m_width/2;
        const int  right = m_center + m_width/2;
        const int  range = right - left;
        if (!mInvert) {
            for (int i=m_min; i<=m_max; i++) {
                int v=0;
                if      (i<=left)   v=0;
                else if (i>=right)  v=255;
                else {
                    v = (int)((i-left)*255.0 / range + 0.5);
                    if      (v<0)    v=0;
                    else if (v>255)  v=255;
                }
                const int j = i - m_min;
                m_lut[j] = v;
            }
        } else {
            for (int i=m_min; i<=m_max; i++) {
                int v=0;
                if      (i<=left)   v=0;
                else if (i>=right)  v=255;
                else {
                    v = (int)((i-left)*255.0 / range + 0.5);
                    if      (v<0)    v=0;
                    else if (v>255)  v=255;
                }
                const int j = i - m_min;
                m_lut[j] = 255-v;
            }
        }
}

void CavassData::init ( void ) {
        //set all data elements to 0.
        // 8/22/07 setting members individually by Dewey Odhner.
        mNext = 0;
        m_xSize = m_ySize = m_zSize = 0;
        m_xSpacing = m_ySpacing = m_zSpacing = 0.;
        m_size = 0;
        m_bytesPerSlice = 0;
        mDataType = ucType;
        m_data = 0;
        mEntireVolumeIsLoaded = false;
        mIsCavassFile = false;
        mIsDicomFile = false;
        mIsImageFile = false;
        mIsAsciiVtkFile = false;
        mIsBinaryVtkFile = false;
        m_vh_initialized = false;
        memset( &m_vh, 0, sizeof(m_vh) );
        m_fname = 0;
        mIsColor = false;
        m_min = 0;
        m_max = 0;
        m_lut = 0;
        mInvert = false;
        m_sliceNo = 0;
        m_center = m_width = 0;
        mR = mG = mB = 0.;
        m_ysub = 0;
        m_zsub = 0;
        //set exceptions (non zero data elements)
        mSamplesPerPixel  = 1;
        mDisplay          = true;
        m_overlay         = true;
        m_scale           = 1;
        mFileOffsetToData = -1;
}

void CavassData::loadImageFile ( bool grayOnly ) {
        mIsImageFile = true;
        m_size = 1;
        m_xSpacing = m_ySpacing = m_zSpacing = 1;  //for lack of something better
        wxImage  temp;
        m_zSize = temp.GetImageCount( m_fname );
        temp.LoadFile( m_fname );
        m_xSize = temp.GetWidth();
        m_ySize = temp.GetHeight();
        m_bytesPerSlice = m_size * m_xSize * m_ySize;
        m_data = (void*)malloc( m_bytesPerSlice * m_zSize );
        assert( m_data != NULL );
        unsigned char*  ucPtr = (unsigned char*)m_data;
        int  out = 0;
        for (int z=0; z<m_zSize; z++) {
            temp.LoadFile( m_fname, wxBITMAP_TYPE_ANY, z );
            unsigned char*  data = temp.GetData();
            int  in = 0;
            for (int y=0; y<m_ySize; y++) {
                for (int x=0; x<m_xSize; x++) {
                    const int  r = data[in++];
                    const int  g = data[in++];
                    const int  b = data[in++];
                    //convert rgb to luminance Y = 0.59G + 0.30R + 0.11B
                    int  luminance = (int)(0.59*g + 0.30*r + 0.11*b + 0.5);
                    if (luminance<0)    luminance = 0;
                    if (luminance>255)  luminance = 255;
                    ucPtr[out++] = luminance;
                }
            }
        }
        mEntireVolumeIsLoaded = true;
}

#ifndef WIN32_TEST
void CavassData::loadImageFileTIFF ( bool grayOnly ) {
        mIsImageFile = true;
        m_size = 1;
        m_xSpacing = m_ySpacing = m_zSpacing = 1;  //for lack of something better

        //each tiff "directory" corresponds to an image.  so let's read through
        // all of the directories to determine the max image width and height
        // and the number of images.
        TIFF*  tif = TIFFOpen( m_fname, "rb" );
        assert( tif );
        m_xSize = m_ySize = m_zSize = 0;
        do {
            uint32  w, h;
            TIFFGetField( tif, TIFFTAG_IMAGEWIDTH,  &w );
            TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &h );
            if (w>(uint32)m_xSize)    m_xSize = w;
            if (h>(uint32)m_ySize)    m_ySize = h;
            m_zSize++;
        } while (TIFFReadDirectory(tif));
        TIFFClose( tif );

        //next, load all of the slice data
        if (!grayOnly)   mSamplesPerPixel = 3;  //indicate color data
        m_bytesPerSlice = m_size * m_xSize * m_ySize * mSamplesPerPixel;
        m_data = (void*)malloc( m_bytesPerSlice * m_zSize );
        assert( m_data != NULL );
        unsigned char*  ucPtr = (unsigned char*)m_data;
        int  out = 0;
        tif = TIFFOpen( m_fname, "rb" );
        assert( tif );
        for (int z=0; z<m_zSize; z++) {
            uint32  w, h;
            TIFFGetField( tif, TIFFTAG_IMAGEWIDTH,  &w );
            TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &h );

            size_t   npixels = w * h;
            uint32*  raster  = (uint32*)malloc( npixels * sizeof (uint32) );
            assert( raster != NULL );
#ifndef  NDEBUG
            int  ret =
#endif
            TIFFReadRGBAImage( tif, w, h, raster, 0 );
            assert( ret == 1 );
            for (int y=0; y<m_ySize; y++) {
                int  in = (h-1-y)*w;  //because tiff stores the image from bottom to top (inverted/flipped about the horizontal/x axis)
                for (int x=0; x<m_xSize; x++) {
                    int  r=0, g=0, b=0;
                    if (x<(int)w && y<(int)h) {
                        r = TIFFGetR( raster[in] );
                        g = TIFFGetG( raster[in] );
                        b = TIFFGetB( raster[in] );
                        ++in;
                    }
                    if (grayOnly) {
                        //convert rgb to luminance Y = 0.59G + 0.30R + 0.11B
                        int  luminance = (int)(0.59*g + 0.30*r + 0.11*b + 0.5);
                        if (luminance<0)    luminance = 0;
                        if (luminance>255)  luminance = 255;
                        ucPtr[out++] = luminance;
                    } else {
                        ucPtr[out++] = r;
                        ucPtr[out++] = g;
                        ucPtr[out++] = b;
                    }
                }  //end for x
            }  //end for y
            free( raster );
            TIFFReadDirectory( tif );
        }  //end for z
        TIFFClose( tif );
        mEntireVolumeIsLoaded = true;
}
#endif

void CavassData::loadVtkFile ( const bool loadHeaderOnly ) {
        //parse the contents of the vtk header
        FILE*  infp  = fopen( m_fname, "rb" );     assert( infp  != NULL );
        char   buff[BUFSIZ], tmp[BUFSIZ];
        //skip the first 2 lines
        fgets( buff, sizeof buff, infp );
        fgets( buff, sizeof buff, infp );

        fgets( buff, sizeof buff, infp );
        if (strcasestr(buff,"BINARY")==buff) {
            mIsAsciiVtkFile  = false;
            mIsBinaryVtkFile = true;
        } else if (strcasestr(buff,"ASCII")==buff) {
            mIsAsciiVtkFile  = true;
            mIsBinaryVtkFile = false;
        } else {
            puts( "error parsing the vtk file at BINARY or ASCII." );
            puts( buff );
            exit( -1 );
        }

        fgets( buff, sizeof buff, infp );
        if (strcasestr(buff,"DATASET STRUCTURED_POINTS")!=buff) {
            puts( "error parsing the vtk file at DATASET." );
            exit( -1 );
        }

        fgets( buff, sizeof buff, infp );
        if (strcasestr(buff,"DIMENSIONS")!=buff) {
            puts( "error parsing the vtk file at DIMENSIONS." );
            exit( -1 );
        }
        int   xSize, ySize, zSize;
        sscanf( buff, "%s %d %d %d", tmp, &xSize, &ySize, &zSize );

        fgets( buff, sizeof buff, infp );
        if (strcasestr(buff,"SPACING")!=buff) {
            puts( "error parsing the vtk file at SPACING." );
            exit( -1 );
        }
        float  xSpace, ySpace, zSpace;
        sscanf( buff, "%s %f %f %f", tmp, &xSpace, &ySpace, &zSpace );

        fgets( buff, sizeof buff, infp );
        if (strcasestr(buff,"ORIGIN")!=buff) {
            puts( "error parsing the vtk file at ORIGIN." );
            exit( -1 );
        }

        fgets( buff, sizeof buff, infp );
        if (strcasestr(buff,"POINT_DATA")!=buff) {
            puts( "error parsing the vtk file at POINT_DATA." );
            exit( -1 );
        }

        //SCALARS scalars unsigned_short 1
        fgets( buff, sizeof buff, infp );
        if (strcasestr(buff,"SCALARS")!=buff) {
            puts( "error parsing the vtk file at SCALARS." );
            exit( -1 );
        }
        char  tmp2[BUFSIZ], dataType[BUFSIZ];
        int   count=0;
        sscanf( buff, "%s %s %s %d", tmp, tmp2, dataType, &count );
        assert( strcasestr(dataType,"int")==dataType ||
                strcasestr(dataType,"unsigned_short")==dataType ||
                strcasestr(dataType,"unsigned_char" )==dataType );
        assert( count==1 );

        fgets( buff, sizeof buff, infp );
        if (strcasestr(buff,"LOOKUP_TABLE default")!=buff) {
            puts( "error parsing the vtk file at LOOKUP_TABLE." );
            exit( -1 );
        }

        //init members
        m_xSize = xSize;      m_ySize = ySize;      m_zSize = zSize;
        m_xSpacing = xSpace;  m_ySpacing = ySpace;  m_zSpacing = zSpace;
        m_size = m_bytesPerSlice = 0;
        if ( strcasestr(dataType,"unsigned_char") == dataType ) {
            m_size = 1;
        } else if ( strcasestr(dataType,"unsigned_short") == dataType ) {
            m_size = 2;
        } else if ( strcasestr(dataType,"int") == dataType ) {
            m_size = 4;
        } else {
            assert( 0 );
        }
        m_bytesPerSlice = m_size * xSize * ySize;

        mFileOffsetToData = ftello( infp );  //remember where data starts
        assert( mFileOffsetToData!=-1 );
        if (loadHeaderOnly) {
            assert( mIsBinaryVtkFile );
            return;
        }

        //allocate space for the entire data set
        assert( m_bytesPerSlice>0 );
        m_data = (void*)malloc( m_bytesPerSlice*m_zSize );
        assert( m_data!=NULL );
        //read in the data
        if (mIsBinaryVtkFile) {
            int  n = fread( m_data, 1, m_bytesPerSlice*m_zSize, infp );
            if (n != m_bytesPerSlice*m_zSize)
			{
				fprintf(stderr, "Failure reading file\n");
				exit(-1);
			}
            assert( m_size==1 || m_size==2 || m_size==4 );
            if (m_size==2) {
                unsigned short*  usptr = (unsigned short*)m_data;
                for (int i=0; i<m_xSize*m_ySize*m_zSize; i++) {
                    swap( &usptr[i] );
                }
            }
        } else if (mIsAsciiVtkFile) {
            if (m_size==1) {
                unsigned char*  ucptr = (unsigned char*)m_data;
                for (int i=0; i<m_xSize*m_ySize*m_zSize; i++) {
                    int  value = 0;
                    int  n = fscanf( infp, "%d", &value );
					if (n != 1) {
						fprintf(stderr, "bad integer format\n");
						exit(-1);
					}
                    ucptr[i] = (unsigned char)value;
                }
            } else if (m_size==2) {
                unsigned short*  usptr = (unsigned short*)m_data;
                for (int i=0; i<m_xSize*m_ySize*m_zSize; i++) {
                    int  value = 0;
                    int  n = fscanf( infp, "%d", &value );
					if (n != 1) {
						fprintf(stderr, "bad integer format\n");
						exit(-1);
					}
                    usptr[i] = (unsigned short)value;
                }
            } else if (m_size==4) {
                int*  iptr = (int*)m_data;
                for (int i=0; i<m_xSize*m_ySize*m_zSize; i++) {
                    int  value = 0;
                    int  n = fscanf( infp, "%d", &value );
					if (n != 1) {
						fprintf(stderr, "bad integer format\n");
						exit(-1);
					}
                    iptr[i] = (int)value;
                }
            } else {
                assert( 0 );
            }
        } else {
            //should never get here.
            assert( 0 );
        }

        fclose( infp );     infp=NULL;
        mEntireVolumeIsLoaded = true;
}

int CavassData::loadCavassFile ( const bool loadHeaderOnly ) {
        mIsCavassFile = true;
        //handle a cavass/3dviewnix file
        FILE*  fp = fopen(m_fname, "rb");
        if (fp==NULL) {
            cerr << "    Can't open " << m_fname << "." << endl;
            wxLogMessage( "    Can't open %s", m_fname );
            return ERR_OPENFILE;
        }
        char  group[5], element[5];
        int  err = VReadHeader(fp, &m_vh, group, element);
        if (err && err<106) {
            cerr << "    Can't read " << m_fname << "'s header." << endl;
            wxLogMessage( "    Can't read %s's header.", m_fname );
            return ERR_READFILE;
        }
        m_vh_initialized = true;
        showHeaderContents();
        if (m_vh.gen.data_type == IMAGE0)
        {
          m_xSize = m_vh.scn.xysize[0];
          m_ySize = m_vh.scn.xysize[1];
          if (m_vh.scn.dimension!=3) {
              cerr << "    Only 3D scenes are currently supported." << endl;
              wxLogMessage( "    Only 3D scenes are currently supported." );
              if (m_vh.scn.dimension!=4) return ERR_ONLYSUPPORT3D;
          }
          m_xSpacing = m_vh.scn.xypixsz[0];
          m_ySpacing = m_vh.scn.xypixsz[1];
          if (m_vh.scn.dimension == 3)
          {
              m_zSize = m_vh.scn.num_of_subscenes[0];
              m_tSize = 1;
              if (m_zSize>1) {
                  m_zSpacing = m_vh.scn.loc_of_subscenes[1]
                      - m_vh.scn.loc_of_subscenes[0];
              } else {
                  m_zSpacing = 1.0;  //better than nothing
              }
          }
          else
          {
              m_zSize = 0;
              m_tSize = m_vh.scn.num_of_subscenes[0];
              for (int j=1; j<=m_tSize; j++)
                  m_zSize += m_vh.scn.num_of_subscenes[j];
              if (m_vh.scn.num_of_subscenes[1] > 1)
                  m_zSpacing = m_vh.scn.loc_of_subscenes[m_tSize+1]
                      - m_vh.scn.loc_of_subscenes[m_tSize];
              else
                  m_zSpacing = 1.0;  //better than nothing
              if (m_tSize > 1)
                  m_tSpacing = m_vh.scn.loc_of_subscenes[1]
                      - m_vh.scn.loc_of_subscenes[0];
              else
                  m_tSpacing = 1.0;  //better than nothing
          }
          switch (m_vh.scn.num_of_bits) {
              case  1 :
              case  8 :  m_size=1;  break;
              case 16 :  m_size=2;  break;
              case 32 :  m_size=4;  break;
              case 64 :  m_size=8;  break;
              default :
                  cerr << "Only 1-, 8-, 16, or 32-bit data are currently supported."
                       << endl;
                  assert( 0 );
                  return ERR_ONLYSUPPORT_1_8_16_32;
                  break;
          }
        }
        else if (m_vh.gen.data_type == MOVIE0)
        {
          m_xSize = m_vh.dsp.xysize[0];
          m_ySize = m_vh.dsp.xysize[1];
          m_xSpacing = m_vh.dsp.xypixsz[0];
          m_ySpacing = m_vh.dsp.xypixsz[1];
          m_zSize = m_vh.dsp.num_of_images;
          m_tSize = 1;
          m_zSpacing = 1;
          mSamplesPerPixel = m_vh.dsp.num_of_elems;
		  m_size = m_vh.dsp.num_of_bits/8;
        }
        m_bytesPerSlice = m_size * m_xSize * m_ySize;

        //prepare to read all of the slices
        err = VSeekData(fp, 0);
        mFileOffsetToData = ftello( fp );  //remember where data starts
        assert( mFileOffsetToData!=-1 );
        if (loadHeaderOnly)    return 0;

        m_data = (void*)malloc( m_bytesPerSlice * m_zSize );
        if (m_data==NULL) {
            cerr << "Out of memory while reading " << m_fname << endl;
            return ERR_OUTOFMEMORY;
        }
        //read all of the slices
        if (m_vh.gen.data_type==MOVIE0 || m_vh.scn.num_of_bits!=1) {
            //gray (more then 1 bit per pixel) data
            int  num;
            err = VReadData( (char*)m_data, m_size, m_zSize*m_bytesPerSlice/m_size,
                             fp, &num );
        } else {  //this is binary (1 bit per pixel) data
            assert( m_vh.scn.dimension_in_alignment == 2 );
            assert( m_vh.scn.bytes_in_alignment     == 1 );

            //unpack the packed binary data bits to either 0 or 1 in 8 bit data
            //allow for widths that are not evenly divisible by 8
            int  bytesPerSlice = m_xSize * m_ySize / 8;
            if ((m_xSize*m_ySize)%8)    ++bytesPerSlice;
            unsigned char*  tmp = (unsigned char*)malloc( bytesPerSlice );
            assert( tmp!=NULL );
            unsigned char*  ucPtr = (unsigned char*)m_data;
            int  i=0;
            for (int z=0; z<m_zSize; z++) {
                memset( tmp, 0, bytesPerSlice );
                int  num;
                err = VReadData( (char*)tmp, 1, bytesPerSlice, fp, &num );
                unpack( &ucPtr[i], tmp, m_xSize, m_ySize );
                i += m_xSize*m_ySize;
            }
            free(tmp);    tmp=NULL;
        }
        VCloseData(fp);  fp=NULL;
        mEntireVolumeIsLoaded = true;

        return 0;
}

void CavassData::showHeaderContents ( void ) {
        if (!m_vh_initialized) {
            cout << "MontageCanvas::showHeaderContents: header not initialized."
                 << endl;
            return;
        }

        #define show(A)                                         \
            if (A ## _valid) {                                  \
                ostringstream  oss;                             \
                oss  << "" #A ": " << A;                        \
                if (mLogLevel >= 1)                             \
					cout << setw(30) << (const char *)oss.str().c_str() << endl;  \
                if (mLogLevel >= 2)                             \
				{                                               \
					wxLogMessage( "%s", (const char *)oss.str().c_str() ); \
				}                                               \
            }

        #define showArray(A,N)                      \
            if (A ## _valid) {                      \
                ostringstream  oss;                 \
                oss << "" #A ": ";                  \
                if (mLogLevel >= 1)                 \
					cout << setw(30) << "" #A ": "; \
                for (int i=0; i<(N); i++) {         \
                    if (mLogLevel >= 1)             \
						cout << A[i] << " ";        \
                    oss << A[i] << " ";             \
                }                                   \
                if (mLogLevel >= 1)                 \
					cout << endl;                   \
                if (mLogLevel >= 2)                 \
				{                                   \
					wxLogMessage( (const char *)oss.str().c_str() );  \
				}                                   \
            }

        //general info
        show( m_vh.gen.study_date      );
        show( m_vh.gen.study_time      );
        show( m_vh.gen.modality        );
        show( m_vh.gen.institution     );
        show( m_vh.gen.physician       );
        show( m_vh.gen.department      );
        show( m_vh.gen.radiologist     );
        show( m_vh.gen.model           );
        show( m_vh.gen.description     );
        show( m_vh.gen.comment         );
        show( m_vh.gen.patient_name    );
        show( m_vh.gen.patient_id      );
        show( m_vh.gen.slice_thickness );
        show( m_vh.gen.study           );
        show( m_vh.gen.series          );
        cout << endl;

        if (m_vh.gen.data_type == IMAGE0)
        {
            //scene info
            show( m_vh.scn.dimension       );
            showArray( m_vh.scn.xysize,  2 );
            showArray( m_vh.scn.num_of_subscenes, m_vh.scn.dimension-2 );
            showArray( m_vh.scn.xypixsz, 2 );
            if (m_vh.scn.dimension==3)
                showArray( m_vh.scn.loc_of_subscenes, m_vh.scn.num_of_subscenes[0] );
            show( m_vh.scn.num_of_bits     );
            show( m_vh.scn.description     );
        }
        else if (m_vh.gen.data_type == MOVIE0)
        {
            //display info
            show( m_vh.dsp.dimension       );
            showArray( m_vh.dsp.xysize,  2 );
            show( m_vh.dsp.num_of_images   );
            showArray( m_vh.dsp.xypixsz, 2 );
            show( m_vh.dsp.num_of_bits     );
            show( m_vh.dsp.description     );
        }
}

void SliceData::init ( void )
     {     
        mFp = 0;       
        pSliceData = NULL;
        nCurSliceNo = -1;

        /** \todo add support for vtk files */
        assert( mIsCavassFile && m_vh_initialized );
		if (m_vh.gen.data_type == IMAGE0)
		{
			if (m_vh.scn.smallest_density_value_valid &&
					m_vh.scn.largest_density_value_valid)
			{	
        		m_min = (int)(m_vh.scn.smallest_density_value[0]);
        		m_max = (int)(m_vh.scn.largest_density_value[0] + 0.5);
			}
			else
			{
				m_min = 0;
				m_max = (1<<m_vh.scn.num_of_bits)-1;
			}
		}
		else
		{
			m_min = 0;
			m_max = 255;
		}

        m_lut = (unsigned char*)malloc( (m_max-m_min+1)*sizeof(unsigned char) );
        assert( m_lut != NULL );
        if (m_min==0 && m_max<=1)
        {
            m_center = 0;
            m_width  = 1;
        }
        else
        {
            m_center = m_min + (m_max-m_min+1)/2;
            m_width  = m_max-m_min+1;
        }
        initLUT();
}

void *SliceData::getSlice ( const int which )
    {
        if (which<0 || which>=m_zSize || mFp==NULL )    return 0; //|| m_data==0	
        if( which == nCurSliceNo && pSliceData != NULL )
            return pSliceData;

        //if (mEntireVolumeIsLoaded) 
        if (mEntireVolumeIsLoaded) 
        {
            if( m_data == NULL )
                return 0;
            //no, not really slice data but determine the start addr of the
            // slice anyway.
            switch (m_size) {
                case 1:
                    {
                        unsigned char*  ucptr = (unsigned char*)m_data;
                        return &ucptr[ m_zsub[which] ];
                    }
                    break;
                case 2:
                    {
                        unsigned short*  usptr = (unsigned short*)m_data;
                        return &usptr[ m_zsub[which] ];
                    }
                    break;
                case 4:
                    {
                        unsigned int*  uiptr = (unsigned int*)m_data;
                        return &uiptr[ m_zsub[which] ];
                    }
                    break;
                default:
                    assert( 0 );
                    break;
            }
            return 0;
        }
       
        //this slice hasn't been loaded yet.  so let's go ahead and load it.
        assert( mFileOffsetToData >= 0 );
        assert( mIsBinaryVtkFile || mIsCavassFile );

        assert( mFileOffsetToData!=-1 );        

        if( pSliceData == NULL )
            pSliceData = (void*)malloc( m_bytesPerSlice );

        if (pSliceData==NULL) 
        {
            cerr << "Out of memory while reading " << endl;
            return 0;
        }
        //read the slices
        if (m_vh.gen.data_type==MOVIE0 || m_vh.scn.num_of_bits!=1) 
        {
            //gray (more then 1 bit per pixel) data
            int err = VLSeekData(mFp, (double)which*m_bytesPerSlice);		

            int  num;
            err = VReadData( (char*)pSliceData, m_size, m_bytesPerSlice/m_size, mFp, &num );
            assert( num == m_bytesPerSlice/m_size );
        } 
        else 
        {  //this is binary (1 bit per pixel) data
            assert( m_vh.scn.dimension_in_alignment == 2 );
            assert( m_vh.scn.bytes_in_alignment     == 1 );

            //allow for widths that are not evenly divisible by 8            
            int  bytesPerSlice = ( m_xSize * m_ySize + 7 ) / 8;
            int err = VLSeekData(mFp, (double)which*bytesPerSlice);

            //unpack the packed binary data bits to either 0 or 1 in 8 bit data
            static const int  masks[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
            unsigned char*  tmp = (unsigned char*)malloc( bytesPerSlice );
            assert( tmp!=NULL );
            unsigned char*  ucPtr = (unsigned char*)pSliceData;
            
            int  i=0;			
            memset( tmp, 0, bytesPerSlice );
            int  num;
            err = VReadData( (char*)tmp, 1, bytesPerSlice, mFp, &num );
            for (int y=0; y<m_ySize; y++) 
            {
                for (int x=0; x<m_xSize; x++) 
                {
                    //if (tmp[y*(m_xSize/8)+x/8] & masks[x%8])
                    const int  s = y * m_xSize + x;
                    if (tmp[s/8] & masks[s%8])
                        ucPtr[i] = 1;
                    else
                        ucPtr[i] = 0;
                    i++;
                }
            }
            free(tmp);    tmp=NULL;
        }

        nCurSliceNo = which;
        return pSliceData;
}

int CavassData::sliceIndex ( const int volume, const int slice_of_volume )
{
	if (m_tSize == 1)
	{
		assert(volume == 0);
		return slice_of_volume;
	}
	assert(m_vh_initialized);
	assert(m_vh.scn.dimension == 4);
	int i, v;
	for (i=v=0; v<volume; v++)
		i += m_vh.scn.num_of_subscenes[v+1];
	return i+slice_of_volume;
}

int CavassData::sliceOfVolume ( const int slice_index )
{
	if (m_tSize == 1)
		return slice_index;
	int i=0, v;
	getVolumeAndSlice(v, i, slice_index);
	return i;
}

int CavassData::volumeOfSlice ( const int slice_index )
{
	if (m_tSize == 1)
		return 0;
	int i=0, v=0;
	getVolumeAndSlice(v, i, slice_index);
	return v;
}

void CavassData::getVolumeAndSlice ( int &volume, int &slice_of_volume,
	const int slice_index )
{
	if (m_tSize == 1)
	{
		volume = 0;
		slice_of_volume = slice_index;
		return;
	}
	assert(m_vh_initialized);
	assert(m_vh.scn.dimension == 4);
	int i, v;
	for (v=0,i=slice_index; v<m_vh.scn.num_of_subscenes[0]; v++)
	{
		if (i < m_vh.scn.num_of_subscenes[1+v])
		{
			volume = v;
			slice_of_volume = i;
			return;
		}
		i -= m_vh.scn.num_of_subscenes[1+v];
	}
	assert(FALSE);
}
