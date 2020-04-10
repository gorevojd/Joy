#ifndef JOY_ANIMATION_H
#define JOY_ANIMATION_H

#include "joy_types.h"
#include "joy_math.h"
#include "joy_assets.h"

struct node_transform{
    v3 T;
    quat R;
    v3 S;
    
    b32 Calculated;
};

struct playing_animation{
    f64 GlobalStart;
    f32 PlaybackRate;
    
    node_transform NodeTransforms[256];
    
    u32 AnimationID;
};

struct animation_controller{
    m44 BoneTransformMatrices[128];
    
    playing_animation* PlayingAnimations[2];
    int PlayingAnimationsCount;
};

struct animation_system{
    f64 GlobalTime;
};


void CalculateToModelTransforms(model_info* Model);

void UpdateModelAnimation(assets* Assets,
                          model_info* Model,
                          playing_animation* PlayingAnim,
                          animation_clip* Animation,
                          f64 CurrentTime);

int UpdateModelBoneTransforms(model_info* Model, 
                              skeleton_info* Skeleton,
                              m44* BoneTransformMatrices);

#endif