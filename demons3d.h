//======================================================================
// file:  demons3d.h
//======================================================================
#ifndef __demons3d_h
#define __demons3d_h

int*  doDemons3DRegistration ( 
    const int xSize1, const int ySize1, const int zSize1,
    const double pixelSizeX1, const double pixelSizeY1,
    const double pixelSizeZ1, const int dataSize1,
    const unsigned char* const ucData1,

    const int xSize2, const int ySize2, const int zSize2,
    const double pixelSizeX2, const double pixelSizeY2,
    const double pixelSizeZ2, const int dataSize2,
    const unsigned char* const ucData2,

    const int iterations=10, const int matchPoints=10,
    const int histogramLevels=200, const double standardDeviation=1.0,
    const bool thresholdAtMean=true );

#endif
//======================================================================

