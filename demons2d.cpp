//======================================================================
// file:  demons2d.cpp
//======================================================================
#include  <assert.h>

//#include  "itkAffineTransform.h"
#include  "itkCenteredAffineTransform.h"
#include  "itkCenteredRigid2DTransform.h"
#include  "itkCenteredTransformInitializer.h"

#include  "itkDemonsRegistrationFilter.h"
#include  "itkHistogramMatchingImageFilter.h"
#include  "itkImage.h"
#include  "itkImageRegistrationMethod.h"
#include  "itkImportImageFilter.h"
#include  "itkMeanSquaresImageToImageMetric.h"
#include  "itkRegularStepGradientDescentOptimizer.h"
#include  "itkResampleImageFilter.h"
#include  "itkTranslationTransform.h"
#include  "itkWarpImageFilter.h"

#include  "demons2d.h"

#define  Dim    2  //dimension
typedef  float  PixelType;
typedef  itk::Image< PixelType, Dim >              ImageType;
typedef  itk::ImportImageFilter< PixelType, Dim >  ImportFilterType;
//----------------------------------------------------------------------
//The following section of code implements a Command observer
//that will monitor the progress of the registration process.

#include  "itkCommand.h"
// #include  "itkGradientDescentOptimizer.h"

//todo: this class needs to be parameterized by OptimizerType
class CommandIterationUpdate : public itk::Command {
    public :
        typedef  CommandIterationUpdate   Self;
        typedef  itk::Command             Superclass;
        typedef  itk::SmartPointer<Self>  Pointer;
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        itkNewMacro ( Self );
    protected :
        CommandIterationUpdate ( void ) { }
    public :
        typedef  itk::RegularStepGradientDescentOptimizer  OptimizerType;
        // typedef  itk::GradientDescentOptimizer  OptimizerType;
        typedef  const OptimizerType*           OptimizerPointer;
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        void Execute ( itk::Object* caller, const itk::EventObject& event ) {
            Execute( (const itk::Object *)caller, event );
        }
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        void Execute ( const itk::Object*       object,
                       const itk::EventObject&  event ) {
            OptimizerPointer optimizer
                = dynamic_cast< OptimizerPointer >( object );
            if (typeid(event) != typeid(itk::IterationEvent))    return;
            std::cout << optimizer->GetCurrentIteration() << "   "
                      << optimizer->GetValue() << "   "
                      << optimizer->GetCurrentPosition();
        }
};
//----------------------------------------------------------------------
/*
this function extracts the specified 2d slice from a 3d array and creates
a 2d slice that itk can deal with.
*/
static ImportFilterType::Pointer  convertSlice ( const int xSize,
    const int ySize, const int zSize,
    const double pixelSizeX, const double pixelSizeY,
    const int dataSize, const unsigned char* const ucData, const int sliceNo,
    const bool normalize=false,  int* min=NULL, int* max=NULL )
{
    assert(Dim==2);

    const ImportFilterType::SizeType  size = { xSize, ySize };
    const unsigned int                numberOfPixels = size[0]*size[1];
    ImportFilterType::Pointer         in = ImportFilterType::New();
    ImportFilterType::IndexType       start;
    start.Fill( 0 );
    ImportFilterType::RegionType      region;
    region.SetIndex( start );
    region.SetSize(  size  );
    in->SetRegion( region );
        const double  origin[Dim]  = {0.0, 0.0};  //x,y coordinates
    in->SetOrigin( origin );
    //along x,y directions:
    const double  spacing[Dim] = { pixelSizeX, pixelSizeY };
    in->SetSpacing( spacing );
    PixelType*  localBuffer = new PixelType[ numberOfPixels ];
    const int  offset = sliceNo * ySize * xSize;
    if        (dataSize == 1) {
        for (int i=0; i<numberOfPixels; i++)
            localBuffer[i] = ucData[i + offset];
    } else if (dataSize == 2) {
        unsigned short*  us = (unsigned short*)ucData;
        for (int i=0; i<numberOfPixels; i++)
            localBuffer[i] = us[i + offset];
    } else if (dataSize == 4) {
        int*  si = (int*)ucData;
        for (int i=0; i<numberOfPixels; i++)
            localBuffer[i] = si[i + offset];
    } else {
        assert(0);
    }

    int  myMin = (int)localBuffer[0];
    int  myMax = (int)localBuffer[0];
    for (int i=1; i<numberOfPixels; i++) {
        if (localBuffer[i]<myMin)  myMin=(int)localBuffer[i];
        if (localBuffer[i]>myMax)  myMax=(int)localBuffer[i];
    }
    std::cout << "my min=" << myMin << std::endl
              << "my max=" << myMax << std::endl;

    if (min!=NULL)    *min = myMin;
    if (max!=NULL)    *max = myMax;
    if (normalize) {
        assert( typeid(PixelType)==typeid(double) || typeid(PixelType)==typeid(float) );
        const PixelType  range = max - min;
        for (int i=0; i<numberOfPixels; i++) {
            localBuffer[i] = (localBuffer[i]-myMin)/range;
        }
    }
    // Note that the last argument of this method
    // specifies who will be responsible for deleting the memory block once it
    // is no longer in use.
    const bool  userPromiseToDeleteTheBuffer = false;
    in->SetImportPointer( localBuffer, numberOfPixels,
                          userPromiseToDeleteTheBuffer );
    in->Update();
    return in;
}
//----------------------------------------------------------------------
/*
this function gets the 2d slice from itk and returns an array of ints.
*/
static int*  retrieveSlice ( const ImageType* const image,
    const bool reverseNormalization=false,  const int min=0, const int max=0 )
{
    assert(Dim==2);

    const int  xSize = image->GetLargestPossibleRegion().GetSize(0);
    const int  ySize = image->GetLargestPossibleRegion().GetSize(1);

    std::cout << "retrieveSlice: xSize=" << xSize << ", ySize=" << ySize
              << std::endl;
    int*  buffer = (int*)malloc(sizeof(int) * xSize * ySize);
    assert(buffer!=NULL);
    typedef  itk::ImageRegionConstIterator< ImageType >  ImageIterator;
    ImageIterator  ii( image, image->GetLargestPossibleRegion() );
    ii.GoToBegin();
    int  i=0;
    if (!reverseNormalization) {
        while (!ii.IsAtEnd()) {
            PixelType v = ii.Get();
            buffer[i++] = (int)v;
            ++ii;
        }
    } else {
        const int  range = max - min;
        while (!ii.IsAtEnd()) {
            PixelType v = ii.Get();
            v = v*range+min;
            buffer[i++] = (int)(v+0.5);
            ++ii;
        }
    }
    return buffer;
}
//----------------------------------------------------------------------
int*  doTranslation2DRegistration (
    const int xSize1, const int ySize1, const int zSize1,
    const double pixelSizeX1, const double pixelSizeY1,
    const int dataSize1,
    const unsigned char* const ucData1, const int sliceNo1,

    const int xSize2, const int ySize2, const int zSize2,
    const double pixelSizeX2, const double pixelSizeY2,
    const int dataSize2,
    const unsigned char* const ucData2, const int sliceNo2,

    const int iterations, const double minStepLength,
    const double maxStepLength, const bool normalize )
{
    ImportFilterType::Pointer  fixed  = convertSlice( xSize1, ySize1, zSize1,
        pixelSizeX1, pixelSizeY1, dataSize1, ucData1, sliceNo1, normalize );

    int  min2, max2;  //so we can undo normalization later (if necessary)
    ImportFilterType::Pointer  moving = convertSlice( xSize2, ySize2, zSize2,
        pixelSizeX2, pixelSizeY2, dataSize2, ucData2, sliceNo2, normalize, &min2, &max2 );

    const double*  fixedSpacing = fixed->GetOutput()->GetSpacing();
    const double*  fixedOrigin  = fixed->GetOutput()->GetOrigin();

    typedef  itk::TranslationTransform< double, Dim >  TransformType;
    typedef  itk::RegularStepGradientDescentOptimizer  OptimizerType;
    typedef  itk::MeanSquaresImageToImageMetric< ImageType, ImageType >
             MetricType;
    typedef  itk::LinearInterpolateImageFunction< ImageType, double >
             InterpolatorType;
    typedef  itk::ImageRegistrationMethod< ImageType, ImageType >
             RegistrationType;

    MetricType::Pointer        metric       = MetricType::New();
    TransformType::Pointer     transform    = TransformType::New();
    OptimizerType::Pointer     optimizer    = OptimizerType::New();
    InterpolatorType::Pointer  interpolator = InterpolatorType::New();
    RegistrationType::Pointer  reg          = RegistrationType::New();
        reg->SetMetric( metric );
        reg->SetOptimizer( optimizer );
        reg->SetTransform( transform );
        reg->SetInterpolator ( interpolator );
        reg->SetFixedImage( fixed->GetOutput() );
        reg->SetMovingImage( moving->GetOutput() );
        reg->SetFixedImageRegion( fixed->GetOutput()->GetBufferedRegion() );

    typedef  RegistrationType::ParametersType  ParametersType;
    ParametersType  initialParameters( transform->GetNumberOfParameters() );
    for (int i=0; i<transform->GetNumberOfParameters(); i++) {
        initialParameters[i] = 0.0;
    }
    reg->SetInitialTransformParameters( initialParameters );
    optimizer->SetNumberOfIterations( iterations );
    optimizer->SetMinimumStepLength( minStepLength );
    optimizer->SetMaximumStepLength( maxStepLength );

    CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
    optimizer->AddObserver( itk::IterationEvent(), observer );

    try {
        reg->StartRegistration();
    } catch ( itk::ExceptionObject& err ) {
        std::cout << "ExceptionObject caught!" << std::endl
                  << err << std::endl;
        return NULL;
    }

    ParametersType  finalParameters = reg->GetLastTransformParameters();
    const double tx = finalParameters[0];
    const double ty = finalParameters[1];
    const unsigned int  numberOfIterations = optimizer->GetCurrentIteration();
    const double  bestValue = optimizer->GetValue();

    std::cout << "  result:" << std::endl
              << "    (tx,ty)=" << tx << "," << ty << std::endl
              << "    iterations =" << numberOfIterations << std::endl
              << "    metric value=" << bestValue << std::endl;
    std::cout << "    final parameters=" << finalParameters << std::endl;

    typedef  itk::ResampleImageFilter< ImageType, ImageType >
             ResampleFilterType;
    TransformType::Pointer  finalTransform = TransformType::New();
    finalTransform->SetParameters( finalParameters );
    ResampleFilterType::Pointer  resample = ResampleFilterType::New();
    resample->SetTransform( finalTransform );
    resample->SetInput( moving->GetOutput() );

    resample->SetSize(
        fixed->GetOutput()->GetLargestPossibleRegion().GetSize() );
    resample->SetOutputOrigin(  fixed->GetOutput()->GetOrigin() );
    resample->SetOutputSpacing( fixed->GetOutput()->GetSpacing() );
    resample->SetDefaultPixelValue( 0 );
    resample->Update();

    return retrieveSlice( resample->GetOutput() );
}
//----------------------------------------------------------------------
int*  doCenteredRigid2DRegistration (
    const int xSize1, const int ySize1, const int zSize1,
    const double pixelSizeX1, const double pixelSizeY1,
    const int dataSize1,
    const unsigned char* const ucData1, const int sliceNo1,

    const int xSize2, const int ySize2, const int zSize2,
    const double pixelSizeX2, const double pixelSizeY2,
    const int dataSize2,
    const unsigned char* const ucData2, const int sliceNo2,

    const int iterations, const double minStepLength,
    const double maxStepLength, const bool normalize )
{
    ImportFilterType::Pointer  fixed  = convertSlice( xSize1, ySize1, zSize1,
        pixelSizeX1, pixelSizeY1, dataSize1, ucData1, sliceNo1, normalize );
    const double*  fixedSpacing = fixed->GetOutput()->GetSpacing();
    const double*  fixedOrigin  = fixed->GetOutput()->GetOrigin();
    const ImageType::SizeType  fixedSize
        = fixed->GetOutput()->GetLargestPossibleRegion().GetSize();

    int  min2, max2;  //so we can undo normalization later (if necessary)
    ImportFilterType::Pointer  moving = convertSlice( xSize2, ySize2, zSize2,
        pixelSizeX2, pixelSizeY2, dataSize2, ucData2, sliceNo2, normalize, &min2, &max2 );
    const double*  movingSpacing = moving->GetOutput()->GetSpacing();
    const double*  movingOrigin  = moving->GetOutput()->GetOrigin();
    const ImageType::SizeType  movingSize
        = moving->GetOutput()->GetLargestPossibleRegion().GetSize();

    typedef  itk::CenteredRigid2DTransform< double >   TransformType;
    typedef  itk::RegularStepGradientDescentOptimizer  OptimizerType;
    typedef  itk::MeanSquaresImageToImageMetric< ImageType, ImageType >
             MetricType;
    typedef  itk::LinearInterpolateImageFunction< ImageType, double >
             InterpolatorType;
    typedef  itk::ImageRegistrationMethod< ImageType, ImageType >
             RegistrationType;

    MetricType::Pointer        metric       = MetricType::New();
    TransformType::Pointer     transform    = TransformType::New();
    OptimizerType::Pointer     optimizer    = OptimizerType::New();
    InterpolatorType::Pointer  interpolator = InterpolatorType::New();
    RegistrationType::Pointer  reg          = RegistrationType::New();
        reg->SetMetric( metric );
        reg->SetOptimizer( optimizer );
        reg->SetTransform( transform );
        reg->SetInterpolator ( interpolator );
        reg->SetFixedImage( fixed->GetOutput() );
        reg->SetMovingImage( moving->GetOutput() );
        reg->SetFixedImageRegion( fixed->GetOutput()->GetBufferedRegion() );

    TransformType::InputPointType  fixedCenter;
    fixedCenter[0] = fixedOrigin[0] + fixedSpacing[0] * fixedSize[0] / 2.0;
    fixedCenter[1] = fixedOrigin[1] + fixedSpacing[1] * fixedSize[1] / 2.0;

    TransformType::InputPointType  movingCenter;
    movingCenter[0] = movingOrigin[0] + movingSpacing[0] * movingSize[0] / 2.0;
    movingCenter[1] = movingOrigin[1] + movingSpacing[1] * movingSize[1] / 2.0;

    transform->SetCenter( fixedCenter );
    transform->SetTranslation( movingCenter - fixedCenter );
    transform->SetAngle( 0.0 );

    reg->SetInitialTransformParameters( transform->GetParameters() );

    typedef  OptimizerType::ScalesType  OptimizerScalesType;
    OptimizerScalesType  optimizerScales( transform->GetNumberOfParameters() );
    const double translationScale = 1.0 / 1000.0;
    optimizerScales[0] = 1.0;
    optimizerScales[1] = translationScale;
    optimizerScales[2] = translationScale;
    optimizerScales[3] = translationScale;
    optimizerScales[4] = translationScale;
    optimizer->SetScales( optimizerScales );
    optimizer->SetMinimumStepLength(  minStepLength );
    optimizer->SetMaximumStepLength(  maxStepLength );
    optimizer->SetNumberOfIterations( iterations    );

    CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
    optimizer->AddObserver( itk::IterationEvent(), observer );

    try {
        reg->StartRegistration();
    } catch ( itk::ExceptionObject& err ) {
        std::cout << "ExceptionObject caught!" << std::endl
                  << err << std::endl;
        return NULL;
    }

    OptimizerType::ParametersType finalParameters =
        reg->GetLastTransformParameters();
    const double  theta = finalParameters[0];
    const double  cx    = finalParameters[1];
    const double  cy    = finalParameters[2];
    const double  tx    = finalParameters[3];
    const double  ty    = finalParameters[4];
    const unsigned int  numberOfIterations = optimizer->GetCurrentIteration();
    const double  bestValue = optimizer->GetValue();
    std::cout << std::endl << "  centered rigid result:" << std::endl
         << "    theta=" << theta << " (cx,cy)=(" << cx << "," << cy
         << ") (tx,ty)=(" << tx << "," << ty << ")"
         << " iterations=" << numberOfIterations << std::endl
         << "   metric value=" << bestValue << std::endl
         << "    final parameters=" << finalParameters << std::endl;

    typedef  itk::ResampleImageFilter< ImageType, ImageType >
             ResampleFilterType;
    TransformType::Pointer  finalTransform = TransformType::New();
    finalTransform->SetParameters( finalParameters );
    ResampleFilterType::Pointer  resample = ResampleFilterType::New();
    resample->SetTransform( finalTransform );
    resample->SetInput( moving->GetOutput() );
    resample->SetSize( fixedSize );
    resample->SetOutputOrigin(  fixedOrigin );
    resample->SetOutputSpacing( fixedSpacing );
    resample->SetDefaultPixelValue( 0 );
    resample->Update();

    return retrieveSlice( resample->GetOutput() );
}
//----------------------------------------------------------------------
int*  doCenteredAffine2DRegistration (
    const int xSize1, const int ySize1, const int zSize1,
    const double pixelSizeX1, const double pixelSizeY1,
    const int dataSize1,
    const unsigned char* const ucData1, const int sliceNo1,

    const int xSize2, const int ySize2, const int zSize2,
    const double pixelSizeX2, const double pixelSizeY2,
    const int dataSize2,
    const unsigned char* const ucData2, const int sliceNo2,

    const int iterations, const double minStepLength,
    const double maxStepLength, const bool normalize )
{
    ImportFilterType::Pointer  fixed  = convertSlice( xSize1, ySize1, zSize1,
        pixelSizeX1, pixelSizeY1, dataSize1, ucData1, sliceNo1, normalize );
    const double*  fixedSpacing = fixed->GetOutput()->GetSpacing();
    const double*  fixedOrigin  = fixed->GetOutput()->GetOrigin();
    const ImageType::SizeType  fixedSize
        = fixed->GetOutput()->GetLargestPossibleRegion().GetSize();

    int  min2, max2;  //so we can undo normalization later (if necessary)
    ImportFilterType::Pointer  moving = convertSlice( xSize2, ySize2, zSize2,
        pixelSizeX2, pixelSizeY2, dataSize2, ucData2, sliceNo2, normalize, &min2, &max2 );
    const double*  movingSpacing = moving->GetOutput()->GetSpacing();
    const double*  movingOrigin  = moving->GetOutput()->GetOrigin();
    //const ImageType::SizeType  movingSize
    //    = moving->GetOutput()->GetLargestPossibleRegion().GetSize();

    typedef  itk::CenteredAffineTransform< double, Dim >   TransformType;
    typedef  itk::RegularStepGradientDescentOptimizer      OptimizerType;
    typedef  itk::MeanSquaresImageToImageMetric< ImageType, ImageType >
             MetricType;
    typedef  itk::LinearInterpolateImageFunction< ImageType, double >
             InterpolatorType;
    typedef  itk::ImageRegistrationMethod< ImageType, ImageType >
             RegistrationType;

    MetricType::Pointer        metric       = MetricType::New();
    TransformType::Pointer     transform    = TransformType::New();
    OptimizerType::Pointer     optimizer    = OptimizerType::New();
    InterpolatorType::Pointer  interpolator = InterpolatorType::New();
    RegistrationType::Pointer  reg          = RegistrationType::New();
        reg->SetMetric( metric );
        reg->SetOptimizer( optimizer );
        reg->SetTransform( transform );
        reg->SetInterpolator ( interpolator );
        reg->SetFixedImage( fixed->GetOutput() );
        reg->SetMovingImage( moving->GetOutput() );
        reg->SetFixedImageRegion( fixed->GetOutput()->GetBufferedRegion() );

    typedef  itk::CenteredTransformInitializer< TransformType,
                 ImageType, ImageType >  TransformInitializerType;
    TransformInitializerType::Pointer  initializer
         = TransformInitializerType::New();
    initializer->SetTransform(   transform );
    initializer->SetFixedImage(  fixed->GetOutput() );
    initializer->SetMovingImage( moving->GetOutput() );
    initializer->MomentsOn();
    initializer->InitializeTransform();

    reg->SetInitialTransformParameters( transform->GetParameters() );

    typedef  OptimizerType::ScalesType  OptimizerScalesType;
    OptimizerScalesType  optimizerScales( transform->GetNumberOfParameters() );
    const double translationScale = 1.0 / 1000.0;
    optimizerScales[0] = optimizerScales[1] = 1.0;
    optimizerScales[2] = optimizerScales[3] = 1.0;
    optimizerScales[4] = optimizerScales[5] = translationScale;
    optimizerScales[6] = optimizerScales[7] = translationScale;
    optimizer->SetScales( optimizerScales );
    optimizer->SetMaximumStepLength(  maxStepLength );
    optimizer->SetMinimumStepLength(  minStepLength );
    optimizer->SetNumberOfIterations( iterations    );
    optimizer->MinimizeOn();

    CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
    optimizer->AddObserver( itk::IterationEvent(), observer );

    try {
        reg->StartRegistration();
    } catch ( itk::ExceptionObject& err ) {
        std::cout << "ExceptionObject caught!" << std::endl
                  << err << std::endl;
        return NULL;
    }

    OptimizerType::ParametersType  finalParameters =
        reg->GetLastTransformParameters();
    const double  cx    = finalParameters[4];
    const double  cy    = finalParameters[5];
    const double  tx    = finalParameters[6];
    const double  ty    = finalParameters[7];
    const unsigned int  numberOfIterations = optimizer->GetCurrentIteration();
    const double  bestValue = optimizer->GetValue();
    std::cout << std::endl
              << "  centered affine result:" << std::endl
              << "    (cx,cy)=(" << cx << "," << cy
              << ") (tx,ty)=" << tx << "," << ty << ")"
              << " iterations=" << numberOfIterations << std::endl
              << "   metric value=" << bestValue << std::endl
              << "    final parameters=" << finalParameters << std::endl;

    typedef  itk::ResampleImageFilter< ImageType, ImageType >
             ResampleFilterType;
    TransformType::Pointer  finalTransform = TransformType::New();
    finalTransform->SetParameters( finalParameters );
    ResampleFilterType::Pointer  resample = ResampleFilterType::New();
    resample->SetTransform( finalTransform );
    resample->SetInput( moving->GetOutput() );
    resample->SetSize( fixedSize );
    resample->SetOutputOrigin(  fixedOrigin );
    resample->SetOutputSpacing( fixedSpacing );
    resample->SetDefaultPixelValue( 0 );
    resample->Update();

    return retrieveSlice( resample->GetOutput() );
}
//----------------------------------------------------------------------
int*  doDemons2DRegistration (
    const int xSize1, const int ySize1, const int zSize1,
    const double pixelSizeX1, const double pixelSizeY1,
    const int dataSize1,
    const unsigned char* const ucData1, const int sliceNo1,

    const int xSize2, const int ySize2, const int zSize2,
    const double pixelSizeX2, const double pixelSizeY2,
    const int dataSize2,
    const unsigned char* const ucData2, const int sliceNo2,

    const int iterations, const int matchPoints,
    const int histogramLevels, const double standardDeviation,
    const bool thresholdAtMean, const bool normalize )
{
    std::cout
        << std::endl << "starting 2d demons registration" << std::endl
        << "    #1    size=(" << xSize1 << "," << ySize1 << "," << zSize1 << ")"
        << std::endl
        << "         bytes="  << dataSize1 << ", slice=" << sliceNo1
        << std::endl
        << "       spacing=(" << pixelSizeX1 << "," << pixelSizeY1 << ")"
        << std::endl
        << "    #2    size=(" << xSize2 << "," << ySize2 << "," << zSize2 << ")"
        << std::endl
        << "       spacing=(" << pixelSizeX2 << "," << pixelSizeY2 << ")"
        << std::endl
        << "         bytes="  << dataSize2 << ", slice=" << sliceNo2
        << std::endl
        << "    iterations="  << iterations << ", match points=" << matchPoints
        << std::endl
        << "    histogram levels=" << histogramLevels
        << ", standard deviations=" << standardDeviation << std::endl
        << "    threshold at mean=" << thresholdAtMean
        << std::endl;

    ImportFilterType::Pointer  fixed  = convertSlice( xSize1, ySize1, zSize1,
        pixelSizeX1, pixelSizeY1, dataSize1, ucData1, sliceNo1, normalize );

    int  min2, max2;  //so we can undo normalization later (if necessary)
    ImportFilterType::Pointer  moving = convertSlice( xSize2, ySize2, zSize2,
        pixelSizeX2, pixelSizeY2, dataSize2, ucData2, sliceNo2, normalize, &min2, &max2 );

    const double*  fixedSpacing = fixed->GetOutput()->GetSpacing();
    const double*  fixedOrigin  = fixed->GetOutput()->GetOrigin();

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
    std::cout << "    linear interpolator" << std::endl;

    WarperType::Pointer  warper = WarperType::New();
    InterpolatorType::Pointer  interpolator = InterpolatorType::New();

    warper->SetInput( moving->GetOutput() );
    warper->SetInterpolator( interpolator );
    warper->SetOutputSpacing( fixedSpacing );
    warper->SetOutputOrigin( fixedOrigin );
    warper->SetDeformationField( filter->GetOutput() );
    warper->Update();

    return retrieveSlice( warper->GetOutput() );
}
//======================================================================

