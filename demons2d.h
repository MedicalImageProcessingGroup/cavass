//======================================================================
// file:  demons2d.h
//======================================================================
#ifndef __demons2d_h
#define __demons2d_h

int*  doDemons2DRegistration ( 
    const int xSize1, const int ySize1, const int zSize1,
    const double pixelSizeX1, const double pixelSizeY1,
    const int dataSize1,
    const unsigned char* const ucData1, const int sliceNo1,

    const int xSize2, const int ySize2, const int zSize2,
    const double pixelSizeX2, const double pixelSizeY2,
    const int dataSize2,
    const unsigned char* const ucData2, const int sliceNo2,

    const int iterations=10, const int matchPoints=10,
    const int histogramLevels=200, const double standardDeviation=1.0,
    const bool thresholdAtMean=true, const bool normalize=false );

#define  T2DR_iterations  500  ///< default iterations
                               ///<
int*  doTranslation2DRegistration (
    const int xSize1, const int ySize1, const int zSize1,
    const double pixelSizeX1, const double pixelSizeY1,
    const int dataSize1,
    const unsigned char* const ucData1, const int sliceNo1,

    const int xSize2, const int ySize2, const int zSize2,
    const double pixelSizeX2, const double pixelSizeY2,
    const int dataSize2,
    const unsigned char* const ucData2, const int sliceNo2,

    const int iterations=T2DR_iterations, const double minStepLength=0.01,
    const double maxStepLength=4.0, const bool normalize=false
);

#define  CR2DR_iterations  500  ///< default iterations
int*  doCenteredRigid2DRegistration (
    const int xSize1, const int ySize1, const int zSize1,
    const double pixelSizeX1, const double pixelSizeY1,
    const int dataSize1,
    const unsigned char* const ucData1, const int sliceNo1,

    const int xSize2, const int ySize2, const int zSize2,
    const double pixelSizeX2, const double pixelSizeY2,
    const int dataSize2,
    const unsigned char* const ucData2, const int sliceNo2,

    const int iterations=CR2DR_iterations, const double minStepLength=0.001,
    const double maxStepLength=0.1, const bool normalize=false );

#define  CA2DR_iterations  500  ///< default iterations
int*  doCenteredAffine2DRegistration (
    const int xSize1, const int ySize1, const int zSize1,
    const double pixelSizeX1, const double pixelSizeY1,
    const int dataSize1,
    const unsigned char* const ucData1, const int sliceNo1,

    const int xSize2, const int ySize2, const int zSize2,
    const double pixelSizeX2, const double pixelSizeY2,
    const int dataSize2,
    const unsigned char* const ucData2, const int sliceNo2,

    const int iterations=CA2DR_iterations, const double minStepLength=0.001,
    const double maxStepLength=0.1, const bool normalize=false );

#endif
//======================================================================

