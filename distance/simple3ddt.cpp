/**
 * \brief this program performs a 3D distance transform via an exhaustive
 * method.
 * distance transform values are output as signed ints (or optionally as
 * doubles).  input and output files are in IM0 format.
 */
#include  <stdlib.h>
#include  <iostream>

#include  <itkImage.h>
#include  "itkIM0VolumeReader.h"
#include  "itkIM0VolumeWriter.h"
#include  "ElapsedTime.h"

#include  "Simple3D.h"

//static int   connectivity = DistanceTransform3D::Face6;
static bool  outputDouble = false;
static bool  symmetric    = false;
//----------------------------------------------------------------------
static void usage ( char* programName ) {
  std::cerr << "Usage:" << std::endl << std::endl
      << "    " << programName << " [-d] [-s] inputImageFile outputDistanceMapImageFile" << std::endl
      << "        " << "-d = output distance map values as doubles" << std::endl
      << "        " << "     (default is " << ::outputDouble << ")" << std::endl
      << "        " << "-s = produce symmetric distance transform"  << std::endl      << "        " << "     (default is " << ::symmetric << ")"    << std::endl
      << std::endl;
  exit( EXIT_FAILURE );
}
//----------------------------------------------------------------------
int main ( int argc, char* argv[] ) {
    ElapsedTime  et;  //timer class

    int  nextArg = 1;
    for ( ; ; ) {
        if (nextArg>=argc)    usage( argv[0] );

        if (strcmp(argv[nextArg],"-d")==0) {
            ::outputDouble = true;
            ++nextArg;
        } else if (strcmp(argv[nextArg],"-s")==0) {
            ::symmetric = true;
            ++nextArg;
        } else if (argv[nextArg][0]=='-') {
            usage( argv[0] );
        } else {
            break;
        }
    }

    //declare the image
    const unsigned int    Dimension = 3;
    typedef  double  InputPixelType;
    typedef  double  OutputPixelType;
    typedef  itk::Image<InputPixelType,  Dimension >  InputImageType;
    typedef  itk::Image<OutputPixelType, Dimension >  OutputImageType;
    InputImageType::Pointer  inputImage = InputImageType::New();

    //declare the reader
    typedef  itk::IM0VolumeReader< InputPixelType, InputImageType >  ImageReaderType;
    ImageReaderType::Pointer  IM0Reader = ImageReaderType::New();

    if (nextArg>=argc)    usage( argv[0] );
	cout << "input file = " << argv[0] << std::endl;
    IM0Reader->SetFileName(  argv[nextArg] );
    ++nextArg;
    IM0Reader->SetInputImage( inputImage );
    IM0Reader->Execute();

    //copy the input data (binary) to a temporary buffer
    itk::ImageRegionIterator< InputImageType >
        it( inputImage, inputImage->GetBufferedRegion() );
    it.GoToBegin();
    InputImageType::SizeType  size = inputImage->GetBufferedRegion().GetSize();
    unsigned char*  d = (unsigned char*)malloc( size[0]*size[1]*size[2]*sizeof(unsigned char) );
    assert( d != NULL );
    int  i = 0;
    for (int z=0; z<(int)size[2]; z++) {
        for (int y=0; y<(int)size[1]; y++) {
            for (int x=0; x<(int)size[0]; x++) {
                assert( !it.IsAtEnd() );
                InputPixelType  value = it.Get();
                if (value>0.5)    d[i++] = 1;
                else              d[i++] = 0;
                ++it;
            }
        }
    }

    //perform the distance transform
    Simple3D  dt( size[0], size[1], size[2] );
    dt.setSymmetricFlag( ::symmetric );
    dt.doTransform( d );

    //copy the results back to the input image (so that we can subsequently
    // just connect input to output and save the results).
    it.GoToBegin();
    i = 0;
    for (int z=0; z<(int)size[2]; z++) {
        for (int y=0; y<(int)size[1]; y++) {
            for (int x=0; x<(int)size[0]; x++) {
                assert( !it.IsAtEnd() );
                it.Set( dt.getD(x,y,z) );
                ++it;
            }
        }
    }

    //declare a writer
    typedef  itk::IM0VolumeWriter< OutputImageType >  WriterType;
    WriterType::Pointer  IM0writer = WriterType::New();

    if (::outputDouble) {
        puts( "output is double" );
        IM0writer->SetOutputDoubleData( true );
    }
    if (nextArg>=argc)    usage( argv[0] );
    IM0writer->SetFileName( argv[nextArg] );
    ++nextArg;
    IM0writer->SetInputImage( inputImage );
    IM0writer->Execute();

    std::cout << "Elapsed time: " << et.getElapsedTime() << "s." << std::endl;

    return 0;
}
//----------------------------------------------------------------------
