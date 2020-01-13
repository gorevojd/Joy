#ifndef JOY_CAMERA_H
#define JOY_CAMERA_H

#include "joy_math.h"
#include "joy_types.h"

struct Game_Camera{
    v3 P;
    v3 dP;
    
    quat Rotation;
};

#endif