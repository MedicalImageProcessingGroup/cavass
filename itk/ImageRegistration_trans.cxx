/*
  Copyright 1993-2008 Medical Image Processing Group
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


#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif


#include "itkImageRegistrationMethod.h"
#include "itkTranslationTransform.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkImage.h"



//#include "itkImageFileReader.h"
//#include "itkImageFileWriter.h"

#include "itkIM0VolumeReader.h"
#include "itkIM0VolumeWriter.h"


#include "itkResampleImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkSubtractImageFilter.h"



class CommandIterationUpdate : public itk::Command 
{
public:
  typedef  CommandIterationUpdate   Self;
  typedef  itk::Command             Superclass;
  typedef itk::SmartPointer<Self>  Pointer;
  itkNewMacro( Self );

protected:
  CommandIterationUpdate() {};

public:

  typedef itk::RegularStepGradientDescentOptimizer     OptimizerType;
  typedef const OptimizerType                         *OptimizerPointer;

  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *)caller, event);
  }

  void Execute(const itk::Object * object, const itk::EventObject & event)
  {
    OptimizerPointer optimizer = 
                         dynamic_cast< OptimizerPointer >( object );

    if( ! itk::IterationEvent().CheckEvent( &event ) )
      {
      return;
      }

    std::cout << optimizer->GetCurrentIteration() << " = ";
    std::cout << optimizer->GetValue() << " : ";
    std::cout << optimizer->GetCurrentPosition() << std::endl;
  }
   
};


int main( int argc, char *argv[] )
{
  if( argc < 4 )
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " fixedImageFile  movingImageFile ";
    std::cerr << "outputImagefile [differenceImageAfter]";
    std::cerr << "[differenceImageBefore]" << std::endl;
    return 1;
    }
  


  const    unsigned int    Dimension = 3;
  typedef  float           PixelType;
 
  typedef itk::Image< PixelType, Dimension >  FixedImageType;
  typedef itk::Image< PixelType, Dimension >  MovingImageType;


  
  typedef itk::TranslationTransform< double, Dimension > TransformType;
 
  typedef itk::RegularStepGradientDescentOptimizer       OptimizerType;
 
  typedef itk::MeanSquaresImageToImageMetric< 
                                    FixedImageType, 
                                    MovingImageType >    MetricType;
 
  typedef itk:: LinearInterpolateImageFunction< 
                                    MovingImageType,
                                    double          >    InterpolatorType;
 
  typedef itk::ImageRegistrationMethod< 
                                    FixedImageType, 
                                    MovingImageType >    RegistrationType;
 
  MetricType::Pointer         metric        = MetricType::New();
  TransformType::Pointer      transform     = TransformType::New();
  OptimizerType::Pointer      optimizer     = OptimizerType::New();
  InterpolatorType::Pointer   interpolator  = InterpolatorType::New();
  RegistrationType::Pointer   registration  = RegistrationType::New();
 
  registration->SetMetric(        metric        );
  registration->SetOptimizer(     optimizer     );
  registration->SetTransform(     transform     );
  registration->SetInterpolator(  interpolator  );
 

  //Declare the reader
  typedef itk::IM0VolumeReader< PixelType, FixedImageType  > FixedImageReaderType;
  typedef itk::IM0VolumeReader< PixelType, MovingImageType > MovingImageReaderType;
  
  FixedImageReaderType::Pointer  IM0FixedImageReader  = FixedImageReaderType::New();
  MovingImageReaderType::Pointer  IM0MovingImageReader  = MovingImageReaderType::New();
  
  
  IM0FixedImageReader->SetFileName(  argv[1] );
  IM0FixedImageReader->SetInputImage(inputImage);
  IM0FixedImageReader->Execute();

  IM0MovingImageReader->SetFileName(  argv[2] );
  IM0MovingImageReader->SetInputImage(inputImage);
  IM0MovingImageReader->Execute();



  typedef itk::ImageFileReader< FixedImageType  > FixedImageReaderType;
  typedef itk::ImageFileReader< MovingImageType > MovingImageReaderType;



  FixedImageReaderType::Pointer  fixedImageReader  = FixedImageReaderType::New();
  MovingImageReaderType::Pointer movingImageReader = MovingImageReaderType::New();

  fixedImageReader->SetFileName(  argv[1] );
  movingImageReader->SetFileName( argv[2] );



  registration->SetFixedImage(    fixedImageReader->GetOutput()    );
  registration->SetMovingImage(   movingImageReader->GetOutput()   );
 
  fixedImageReader->Update();
  registration->SetFixedImageRegion( 
                    fixedImageReader->GetOutput()->GetBufferedRegion() );
 
  typedef RegistrationType::ParametersType ParametersType;
  ParametersType initialParameters( transform->GetNumberOfParameters() );

  initialParameters[0] = 0.0;  // Initial offset in mm along X
  initialParameters[1] = 0.0;  // Initial offset in mm along Y
  
  registration->SetInitialTransformParameters( initialParameters );
  
  optimizer->SetMaximumStepLength( 4.00 );  
  optimizer->SetMinimumStepLength( 0.01 );
 
  optimizer->SetNumberOfIterations( 200 );
 


  // Connect an observer
  CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
  optimizer->AddObserver( itk::IterationEvent(), observer );


 
  try 
    { 
    registration->Update(); 
    } 
  catch( itk::ExceptionObject & err ) 
    { 
    std::cerr << "ExceptionObject caught !" << std::endl; 
    std::cerr << err << std::endl; 
    return -1;
    } 
  
  ParametersType finalParameters = registration->GetLastTransformParameters();
  
  const double TranslationAlongX = finalParameters[0];
  const double TranslationAlongY = finalParameters[1];
  
  const unsigned int numberOfIterations = optimizer->GetCurrentIteration();
  
  const double bestValue = optimizer->GetValue();
 


  // Print out results
  //
  std::cout << "Result = " << std::endl;
  std::cout << " Translation X = " << TranslationAlongX  << std::endl;
  std::cout << " Translation Y = " << TranslationAlongY  << std::endl;
  std::cout << " Iterations    = " << numberOfIterations << std::endl;
  std::cout << " Metric value  = " << bestValue          << std::endl;


  


  
  typedef itk::ResampleImageFilter< 
                            MovingImageType, 
                            FixedImageType >    ResampleFilterType;
  
  ResampleFilterType::Pointer resampler = ResampleFilterType::New();
  resampler->SetInput( movingImageReader->GetOutput() );
  



  
  resampler->SetTransform( registration->GetOutput()->Get() );
  
  FixedImageType::Pointer fixedImage = fixedImageReader->GetOutput();
  resampler->SetSize( fixedImage->GetLargestPossibleRegion().GetSize() );
  resampler->SetOutputOrigin(  fixedImage->GetOrigin() );
  resampler->SetOutputSpacing( fixedImage->GetSpacing() );
  resampler->SetDefaultPixelValue( 100 );
  
  typedef unsigned char OutputPixelType;
  typedef itk::Image< OutputPixelType, Dimension > OutputImageType;
  typedef itk::CastImageFilter< 
                        FixedImageType,
                        OutputImageType > CastFilterType;
  typedef itk::ImageFileWriter< OutputImageType >  WriterType;
 
  WriterType::Pointer      writer =  WriterType::New();
  CastFilterType::Pointer  caster =  CastFilterType::New();
  // Software Guide : EndCodeSnippet


  writer->SetFileName( argv[3] );


 
  caster->SetInput( resampler->GetOutput() );
  writer->SetInput( caster->GetOutput()   );
  writer->Update();
  
  typedef itk::SubtractImageFilter< 
                                  FixedImageType, 
                                  FixedImageType, 
                                  FixedImageType > DifferenceFilterType;

  DifferenceFilterType::Pointer difference = DifferenceFilterType::New();

  difference->SetInput1( fixedImageReader->GetOutput() );
  difference->SetInput2( resampler->GetOutput() );
  
  typedef itk::RescaleIntensityImageFilter< 
                                  FixedImageType, 
                                  OutputImageType >   RescalerType;

  RescalerType::Pointer intensityRescaler = RescalerType::New();
  
  intensityRescaler->SetInput( difference->GetOutput() );
  intensityRescaler->SetOutputMinimum(   0 );
  intensityRescaler->SetOutputMaximum( 255 );

  resampler->SetDefaultPixelValue( 1 );
  
  WriterType::Pointer writer2 = WriterType::New();
  writer2->SetInput( intensityRescaler->GetOutput() );  
  


  if( argc > 4 )
    {
    writer2->SetFileName( argv[4] );
    writer2->Update();
    }



  
  TransformType::Pointer identityTransform = TransformType::New();
  identityTransform->SetIdentity();
  resampler->SetTransform( identityTransform );
  


  if( argc > 5 )
    {
    writer2->SetFileName( argv[5] );
    writer2->Update();
    }

  return 0;
}

