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

void UpdateModelAnimation(assets* Assets,
                          model_info* Model, 
                          animation_clip* Animation,
                          f64 CurrentTime);

int UpdateModelBoneTransforms(model_info* Model, 
                              skeleton_info* Skeleton,
                              m44* BoneTransformMatrices);

#endif