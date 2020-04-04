#ifndef JOY_ANIMATION_H
#define JOY_ANIMATION_H

#include "joy_types.h"
#include "joy_math.h"
#include "joy_assets.h"

struct playing_animation{
    f64 GlobalStart;
    f32 PlaybackRate;
    
    u32 AnimationID;
    
    b32 IsLooping;
};

void AnimateModel(assets* Assets,
                  model_info* Model, 
                  f64 CurrentTime);

#endif