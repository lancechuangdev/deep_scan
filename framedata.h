#ifndef FRAMEDATA_H
#define FRAMEDATA_H

#include "MvCameraControl.h"

struct FrameData {
    unsigned char* pData;
    MV_FRAME_OUT_INFO_EX* pFrameMetadata;

    // Default constructor
    FrameData() : pData(nullptr), pFrameMetadata(nullptr) {}

    // Constructor
    FrameData(unsigned char* data, MV_FRAME_OUT_INFO_EX* metadata)
        : pData(data), pFrameMetadata(metadata) {}
};

#endif // FRAMEDATA_H
