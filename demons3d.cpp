//======================================================================
// file:  demons3d.cpp
//======================================================================
#include  "itkDemonsRegistrationFilter.h"
#include  "itkHistogramMatchingImageFilter.h"
#include  "itkImage.h"
#include  "itkImportImageFilter.h"
#include  "itkWarpImageFilter.h"

#include  <assert.h>
#include  "demons3d.h"

#define  Dim    3  //dimension
typedef  float  PixelType;
typedef  itk::Image< PixelType, Dim >              ImageType;
typedef  itk::ImportImageFilter< PixelType, Dim >  ImportFilterType;
//----------------------------------------------------------------------
static ImportFilterType::Pointer  convertScene ( const int xSize,
    const int ySize, const int zSize, const double pixelSizeX,
    const double pixelSizeY, const double pixelSizeZ,
    const int dataSize, const unsigned char* const ucData )
{
    assert(Dim==3);

    //const ImportFilterType::SizeType  size = { xSize, ySize, zSize };
    ImportFilterType::SizeType  size;
    size[0]=xSize;  size[1]=ySize; size[2]=zSize;
    const unsigned int                numberOfPixels = size[0]*size[1]*size[2];
    ImportFilterType::Pointer         in = ImportFilterType::New();
    ImportFilterType::IndexType       start;
    start.Fill( 0 );
    ImportFilterType::RegionType      region;
    region.SetIndex( start );
    region.SetSize(  size  );
    in->SetRegion( region );
        const double  origin[Dim]  = {0.0, 0.0, 0.0};  //x,y,z coordinates
    in->SetOrigin( origin );
    const double  spacing[Dim] = { pixelSizeX, pixelSizeY, pixelSizeZ };
    //along x,y,z directions
    in->SetSpacing( spacing );
    PixelType*  localBuffer = new PixelType[ numberOfPixels ];
    if        (dataSize == 1) {
        for (int i=0; i<(int)numberOfPixels; i++)
            localBuffer[i] = ucData[i];
    } else if (dataSize == 2) {
        unsigned short*  us = (unsigned short*)ucData;
        for (int i=0; i<(int)numberOfPixels; i++)
            localBuffer[i] = us[i];
    } else if (dataSize == 4) {
        int*  si = (int*)ucData;
        for (int i=0; i<(int)numberOfPixels; i++)
            localBuffer[i] = si[i];
    } else {
        assert(0);
    }

    PixelType  myMin = localBuffer[0];
    PixelType  myMax = localBuffer[0];
    for (int i=1; i<(int)numberOfPixels; i++) {
        if (localBuffer[i]<myMin)  myMin=localBuffer[i];
        if (localBuffer[i]>myMax)  myMax=localBuffer[i];
    }
    std::cout << "my min=" << myMin << std::endl
              << "my max=" << myMax << std::endl;

    // Note that the last argument of this method
    // specifies who will be responsible for deleting the memory block once it
    // is no longer in use.
    const bool  userPromiseToDeleteTheBuffer = false;
    in->SetImportPointer( localBuffer, numberOfPixels,
                          userPromiseToDeleteTheBuffer );

    return in;
}
//----------------------------------------------------------------------
static int*  retrieveScene ( const ImageType* const image ) {
    assert(Dim==3);

    const int  xSize = image->GetLargestPossibleRegion().GetSize(0);
    const int  ySize = image->GetLargestPossibleRegion().GetSize(1);
    const int  zSize = image->GetLargestPossibleRegion().GetSize(2);

    std::cout << "retrieveScene: xSize=" << xSize << ", ySize=" << ySize
              << ", zSize=" << zSize << std::endl;
    int*  buffer = (int*)malloc(sizeof(int) * xSize * ySize * zSize);
    assert(buffer!=NULL);
    typedef  itk::ImageRegionConstIterator< ImageType >  ImageIterator;
    ImageIterator  ii( image, image->GetLargestPossibleRegion() );
    ii.GoToBegin();
    int  i=0;
    while (!ii.IsAtEnd()) {
        PixelType v = ii.Get();
        buffer[i++] = (int)v;
        ++ii;
    }

    return buffer;
}
//----------------------------------------------------------------------
int*  doDemons3DRegistration ( 
    const int xSize1, const int ySize1, const int zSize1,
    const double pixelSizeX1, const double pixelSizeY1,
    const double pixelSizeZ1, const int dataSize1,
    const unsigned char* const ucData1,

    const int xSize2, const int ySize2, const int zSize2,
    const double pixelSizeX2, const double pixelSizeY2,
    const double pixelSizeZ2, const int dataSize2,
    const unsigned char* const ucData2,

    const int iterations, const int matchPoints, const int histogramLevels,
    const double standardDeviation, const bool thresholdAtMean )
{
    ImportFilterType::Pointer  fixed  = convertScene( xSize1, ySize1, zSize1,
        pixelSizeX1, pixelSizeY1, pixelSizeZ1, dataSize1, ucData1 );

    ImportFilterType::Pointer  moving = convertScene( xSize2, ySize2, zSize2,
        pixelSizeX2, pixelSizeY2, pixelSizeZ2, dataSize2, ucData2 );

    fixed->Update();
    const double*  fixedSpacing = fixed->GetOutput()->GetSpacing();
    const double*  fixedOrigin  = fixed->GetOutput()->GetOrigin();
    moving->Update();

    std::cout << std::endl << "starting 3d demons registration" << std::endl;
    typedef  itk::HistogramMatchingImageFilter< ImageType, ImageType >
             MatchingFilterType;
    MatchingFilterType::Pointer  matcher = MatchingFilterType::New();
    matcher->SetInput( moving->GetOutput() );
    matcher->SetReferenceImage( fixed->GetOutput() );
    matcher->SetNumberOfHistogramLevels( histogramLevels );
    matcher->SetNumberOfMatchPoints( matchPoints );
    if (thresholdAtMean)    matcher->ThresholdAtMeanIntensityOn();
    else                    matcher->ThresholdAtMeanIntensityOff();

    typedef  itk::Vector< float, Dim >  VectorPixelType;
    typedef  itk::Image<  VectorPixelType, Dim >  DeformationFieldType;
    typedef  itk::DemonsRegistrationFilter< ImageType, ImageType,
                                            DeformationFieldType >
             RegistrationFilterType;
    RegistrationFilterType::Pointer  filter = RegistrationFilterType::New();
    filter->SetFixedImage( fixed->GetOutput() );
    filter->SetMovingImage( matcher->GetOutput() );
    filter->SetNumberOfIterations( iterations );
    filter->SetStandardDeviations( standardDeviation );
    filter->Update();

    typedef  itk::WarpImageFilter< ImageType, ImageType, DeformationFieldType >
        WarperType;
    typedef  itk::LinearInterpolateImageFunction< ImageType, double >
        InterpolatorType;
    std::cout << "linear interpolator" << std::endl;

    WarperType::Pointer  warper = WarperType::New();
    InterpolatorType::Pointer  interpolator = InterpolatorType::New();

    warper->SetInput( moving->GetOutput() );
    warper->SetInterpolator( interpolator );
    warper->SetOutputSpacing( fixedSpacing );
    warper->SetOutputOrigin( fixedOrigin );
    warper->SetDeformationField( filter->GetOutput() );
    warper->Update();

    return retrieveScene( warper->GetOutput() );
}
//======================================================================

