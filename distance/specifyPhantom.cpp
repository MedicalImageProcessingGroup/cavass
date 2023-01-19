//----------------------------------------------------------------------
/**
 * \brief this file contains code for a program that creates a binary
 * distance transform phantom (.IM0 file).
 */
//----------------------------------------------------------------------
#include  <assert.h>
#include  <stdio.h>

extern "C" {
    #include  "Viewnix.h"
    int VWriteHeader ( FILE* fp, ViewnixHeader* vh, char group[5],
                       char element[5] );
    int VWriteData ( char* data, int size, int items, FILE* fp,
                     int* items_written );
}

static int  dXSize = 100;
static int  dYSize = 100;
static int  dZSize = 100;
static int  dN = (dXSize*dYSize*dZSize);  //size of 3d volume
#define  dXSpace  1.0  //physical space in the x direction
#define  dYSpace  1.0  //physical space in the y direction
#define  dZSpace  1.0  //physical space in the z direction
#define  dMin     0    //value for "outside" an object
#define  dMax     255  //value for "inside" an object

//note: since these are global, they are initialized to 0!
static ViewnixHeader   vh;
static unsigned char*  data;
//static unsigned char  data[dSize][dSize][dSize];
//----------------------------------------------------------------------
static void initHeader ( char* fname ) {
  strcpy( vh.gen.recognition_code, "VIEWNIX1.0" );
  vh.gen.recognition_code_valid = 1;

  vh.gen.data_type = IMAGE0;
  vh.gen.data_type_valid = 1;

  vh.scn.dimension = 3;
  vh.scn.dimension_valid = 1;

  /* set image size */
  vh.scn.xysize[0] = dXSize;
  vh.scn.xysize[1] = dYSize;
  vh.scn.xysize_valid = 1;

  vh.scn.num_of_subscenes = (short*)malloc( 1*sizeof(short) );
  vh.scn.num_of_subscenes[0] = dZSize;
  vh.scn.num_of_subscenes_valid = 1;

  /* set image spacing */
  vh.scn.xypixsz[0] = dXSpace;
  vh.scn.xypixsz[1] = dYSpace;
  vh.scn.xypixsz_valid = 1;

  vh.scn.loc_of_subscenes = (float*)malloc( vh.scn.num_of_subscenes[0]
                                            * sizeof(float) );
  vh.scn.loc_of_subscenes[0] = 0;
  for (int i=1; i<vh.scn.num_of_subscenes[0]; i++) {
      vh.scn.loc_of_subscenes[i] = (float)(vh.scn.loc_of_subscenes[i-1] + dZSpace);
  }
  vh.scn.loc_of_subscenes_valid = 1;

  vh.scn.smallest_density_value = (float*)malloc( sizeof(float) );
  vh.scn.largest_density_value  = (float*)malloc( sizeof(float) );
  vh.scn.smallest_density_value[0] = dMin;
  vh.scn.largest_density_value[0]  = dMax;
  vh.scn.smallest_density_value_valid = 1;
  vh.scn.largest_density_value_valid  = 1;

  vh.scn.num_of_bits = 8;
  vh.scn.num_of_bits_valid = 1;

  vh.scn.num_of_density_values = 1;
  vh.scn.num_of_density_values_valid = 1;

  vh.scn.num_of_integers = vh.scn.num_of_density_values;
  vh.scn.num_of_integers_valid = 1;

  vh.scn.bit_fields = (short*)malloc( 2 * vh.scn.num_of_density_values
                                      * sizeof(short) );
  vh.scn.bit_fields[0] = vh.scn.num_of_bits;
  vh.scn.bit_fields[1] = vh.scn.num_of_bits - 1;
  vh.scn.bit_fields_valid = 1;

  strcpy( vh.gen.filename, fname );
  vh.gen.filename_valid = 1;
}
//----------------------------------------------------------------------
//make sure that the point inside an object is within bounds.
//we intentionally offset by a small amount so that an object point never
// occurs at the edge of the volume.
static bool inBounds ( int x, int y, int z ) {
    const int  inset = 1;
    if (x<inset || y<inset || z<inset)    return false;
    if (x>dXSize-inset-1 || y>dYSize-inset-1 || z>dZSize-inset-1)    return false;
    return true;
}
//----------------------------------------------------------------------
static void usage ( char* programName ) {
  fprintf( stderr, "\nUsage: \n%s [X Y Z] outputImageFile \n", programName );
  fprintf( stderr, "     X Y Z = the size of the output (XxYxZ) (default=%dx%dx%d) \n", dXSize, dYSize, dZSize );
  fprintf( stderr, "     Note: to correspond with display, values range from [1..X,1..Y,1..Z]. \n" );
  exit( EXIT_FAILURE );
}
//----------------------------------------------------------------------
int main ( int argc, char* argv[] ) {
    //make sure that the name of an output file is specified
    if (argc<2)    usage( argv[0] );

    int  nextArg = 1;
    if (argc==5) {
        ::dXSize = atoi( argv[nextArg++] );
        ::dYSize = atoi( argv[nextArg++] );
        ::dZSize = atoi( argv[nextArg++] );
        ::dN =  dXSize * dYSize * dXSize;
    }
    assert( ::dXSize>0 && ::dYSize>0 && ::dZSize>0 );

    char*  env = getenv( "VIEWNIX_ENV" );
    if (env==NULL) {
        printf( "VIEWNIX_ENV not set! \n" );
#if defined (WIN32) || defined (_WIN32)
        printf( "setting VIEWNIX_ENV to C:\\Program Files\\CAVASS \n" );
        _putenv( "VIEWNIX_ENV=C:\\Program Files\\CAVASS" );
#else
        printf( "can't continue. \n" );
        exit( 0 );
#endif
    }

    //allocate the data array
    data = (unsigned char*)malloc( dXSize * dYSize * dZSize * sizeof(unsigned char) );
    assert( data != NULL );
    //initialize the data array to empty
    for (int i=0; i<dXSize*dYSize*dZSize; i++)    data[i] = dMin;

    int  count = 0;
    for ( ; ; ) {
        //read triples of x y z coordinates
        int  x=-1, y=-1, z=-1;
        int  n = fscanf( stdin, "%d %d %d", &x, &y, &z );
        if (n!=3)    break;
        //values are input in 1..size and need to be converted to 0..size-1
        // for array subscripting.  input is this way to correspond to display
        // in montage.
        --x;    --y;    --z;
        if (inBounds(x,y,z)) {
            if (x==0 || y==0 || z==0 || x==dXSize-1 || y==dYSize-1 || z==dZSize-1) {
                fprintf( stderr, "Warning: point in \"periphery\" set. \n" );
            }
            int  sub = z*dXSize*dYSize + y*dXSize + x;
            if (data[sub]==dMin) {
                data[sub] = dMax;
                ++count;
            } else {
                fprintf( stderr, "point set previously. \n" );
            }
        } else {
            fprintf( stderr, "specified point is outside the volume. \n" );
        }
    }
    printf( "set %d unique points. \n", count );

    //initialize the 3dviewnix/cavass header
    assert( argc >= 2 );
    initHeader( argv[nextArg] );

    //write the header
    assert( argc >= 2 );
    FILE*  fp = fopen( argv[nextArg++], "wb" );    assert( fp != NULL );
    char  group[5], element[5];
    int  error_code = VWriteHeader( fp, &vh, group, element );
    //printf( "VWriteHeader returned %d. \n", error_code );

    //write the data
    fprintf( stderr, "writing data \n" );
    int  n;
    error_code = VWriteData( (char*)data, 1, dXSize*dYSize*dZSize, fp, &n );
    //printf( "VWriteData returned %d. \n", error_code );

    fclose( fp );    fp = NULL;

    fprintf( stderr, "done \n" );
    return 0;
}
//----------------------------------------------------------------------
