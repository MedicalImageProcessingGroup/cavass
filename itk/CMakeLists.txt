PROJECT(ITKPrograms)

# Find ITK.
FIND_PACKAGE(ITK)
IF(ITK_FOUND)
  INCLUDE(${ITK_USE_FILE})
ELSE(ITK_FOUND)
  MESSAGE(FATAL_ERROR
          "Cannot build without ITK.  Please set ITK_DIR.")
ENDIF(ITK_FOUND)


INCLUDE_DIRECTORIES(
/usr/3dviewnix/INCLUDE
)

ADD_EXECUTABLE(IM02Meta IM02Meta.cxx)
ADD_EXECUTABLE(BIM2Meta BIM2Meta.cxx)
ADD_EXECUTABLE(Meta2IM0 Meta2IM0.cxx)
ADD_EXECUTABLE(itkMeanFilter itkMeanFilter.cxx)
ADD_EXECUTABLE(itkMedianFilter itkMedianFilter.cxx)
ADD_EXECUTABLE(itkRescaleFilter itkRescaleFilter.cxx)
ADD_EXECUTABLE(itkCannyEdgeDetectionFilter itkCannyEdgeDetectionFilter.cxx)
ADD_EXECUTABLE(itkBinaryThresholdFilter itkBinaryThresholdFilter.cxx)
ADD_EXECUTABLE(itkThresholdFilter itkThresholdFilter.cxx)
ADD_EXECUTABLE(itkSigmoidFilter itkSigmoidFilter.cxx)
ADD_EXECUTABLE(itkGradientMagnitudeFilter itkGradientMagnitudeFilter.cxx)
ADD_EXECUTABLE(itkGradientMagnitudeRecursiveGaussianFilter itkGradientMagnitudeRecursiveGaussianFilter.cxx)
ADD_EXECUTABLE(itkDerivativeFilter itkDerivativeFilter.cxx)
ADD_EXECUTABLE(itkBinaryErodeFilter itkBinaryErodeFilter.cxx)
ADD_EXECUTABLE(itkBinaryDilateFilter itkBinaryDilateFilter.cxx)
ADD_EXECUTABLE(itkGrayErodeFilter itkGrayErodeFilter.cxx)
ADD_EXECUTABLE(itkGrayDilateFilter itkGrayDilateFilter.cxx)
ADD_EXECUTABLE(itkDiscreteGaussianFilter itkDiscreteGaussianFilter.cxx)
ADD_EXECUTABLE(itkBinomialBlurFilter itkBinomialBlurFilter.cxx)
ADD_EXECUTABLE(itkSmoothRecursiveGaussianFilter itkSmoothRecursiveGaussianFilter.cxx)
ADD_EXECUTABLE(itkGradientAnisotropicDiffusionFilter itkGradientAnisotropicDiffusionFilter.cxx)
ADD_EXECUTABLE(itkCurvatureAnisotropicDiffusionFilter itkCurvatureAnisotropicDiffusionFilter.cxx)
ADD_EXECUTABLE(itkCurvatureFlowImageFilter itkCurvatureFlowImageFilter.cxx)
ADD_EXECUTABLE(itkVotingBinaryHoleFillingFilter itkVotingBinaryHoleFillingFilter.cxx)
ADD_EXECUTABLE(itkBinaryMedianFilter itkBinaryMedianFilter.cxx)
ADD_EXECUTABLE(itkDanielssonDistanceMapFilter itkDanielssonDistanceMapFilter.cxx)
ADD_EXECUTABLE(itkSignedDanielssonDistanceMapFilter itkSignedDanielssonDistanceMapFilter.cxx)
ADD_EXECUTABLE(itkSignedMaurerDistanceMapFilter itkSignedMaurerDistanceMapFilter.cxx)


TARGET_LINK_LIBRARIES(IM02Meta
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(BIM2Meta
   ITKBasicFilters
   ITKIO
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(Meta2IM0
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)

TARGET_LINK_LIBRARIES(itkMeanFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)

TARGET_LINK_LIBRARIES(itkMedianFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkRescaleFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkCannyEdgeDetectionFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)

TARGET_LINK_LIBRARIES(itkBinaryThresholdFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkThresholdFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkSigmoidFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkGradientMagnitudeFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkGradientMagnitudeRecursiveGaussianFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkDerivativeFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkBinaryErodeFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkBinaryDilateFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkGrayErodeFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkGrayDilateFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkDiscreteGaussianFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkBinomialBlurFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkSmoothRecursiveGaussianFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkGradientAnisotropicDiffusionFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkCurvatureAnisotropicDiffusionFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkCurvatureFlowImageFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkVotingBinaryHoleFillingFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkBinaryMedianFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkDanielssonDistanceMapFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
TARGET_LINK_LIBRARIES(itkSignedDanielssonDistanceMapFilter
   ITKBasicFilters  
   ITKIO 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)

TARGET_LINK_LIBRARIES(itkSignedMaurerDistanceMapFilter
   ITKBasicFilters  
   ITKIO 
   itkvnl	 
   /usr/3dviewnix/LIBRARY/libviewnixg.a
   /usr/3dviewnix/LIBRARY/libviewnixd.a 
)
