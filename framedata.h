#ifndef FRAMEDATA_H
#define FRAMEDATA_H

#include "MvCameraControl.h"

struct FrameData {
    unsigned char* pData;
    MV_FRAME_OUT_INFO_EX* pMetadata;

    // Default constructor
    FrameData() : pData(nullptr), pMetadata(nullptr) {}

    // Constructor
    FrameData(unsigned char* data, MV_FRAME_OUT_INFO_EX* metadata)
        : pData(data), pMetadata(metadata) {}
};

#endif // FRAMEDATA_H
