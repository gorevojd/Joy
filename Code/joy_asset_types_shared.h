#ifndef JOY_ASSET_TYPES_SHARED_H
#define JOY_ASSET_TYPES_SHARED_H

#include "joy_types.h"
#include "joy_math.h"

struct bone_info{
    char Name[256];
    
    m44 InvBindPose;
    
    int ParentIndex;
    int FirstChildIndex;
    int ChildCount;
};


#endif