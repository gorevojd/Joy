#include "joy_animation.h"

INTERNAL_FUNCTION int FindPrevFrameIndexForVectorKey(animation_vector_key* Keys, 
                                                     int KeysCount, 
                                                     f32 CurTickTime)
{
    int Result = -1;
    
    for(int KeyIndex = 0;
        KeyIndex < KeysCount;
        KeyIndex++)
    {
        animation_vector_key* Key = &Keys[KeyIndex];
        
        if(Key->Time < CurTickTime){
            Result = KeyIndex;
            
            break;
        }
    }
    
    return(Result);
}

INTERNAL_FUNCTION int FindPrevFrameIndexForQuatKey(animation_quaternion_key* Keys,
                                                   int KeysCount,
                                                   f32 CurTickTime)
{
    int Result = -1;
    
    for(int KeyIndex = 0;
        KeyIndex < KeysCount;
        KeyIndex++)
    {
        animation_quaternion_key* Key = &Keys[KeyIndex];
        
        if(Key->Time < CurTickTime){
            Result = KeyIndex;
            
            break;
        }
    }
    
    return(Result);
}


INTERNAL_FUNCTION v3 GetAnimatedVector(animation_clip* Animation,
                                       animation_vector_key* Keys,
                                       int KeysCount,
                                       f32 CurTickTime,
                                       v3 DefaultValue)
{
    v3 Result = DefaultValue;
    
    // NOTE(Dima): Loading first frame's values
    if(KeysCount){
        Result = Keys[0].Value;
        
        // NOTE(Dima): Finding frame index before CurTickTime
        int FoundPrevIndex = FindPrevFrameIndexForVectorKey(Keys,
                                                            KeysCount,
                                                            CurTickTime);
        
        if(FoundPrevIndex != -1){
            if((FoundPrevIndex != KeysCount) || Animation->IsLooping)
            {
                // NOTE(Dima): If not last key frame
                animation_vector_key* PrevKey = &Keys[FoundPrevIndex];
                
                animation_vector_key* NextKey = 0;
                f32 TickDistance = 0.0f;
                
                // NOTE(Dima): If found frame is not last
                if(FoundPrevIndex != KeysCount)
                {
                    NextKey = &Keys[FoundPrevIndex + 1];
                    
                    TickDistance = NextKey->Time - PrevKey->Time;
                }
                
                // NOTE(Dima): If animation is looping and founf frame is last
                if(Animation->IsLooping && (FoundPrevIndex == KeysCount))
                {
                    NextKey = &Keys[0];
                    
                    TickDistance = Animation->DurationTicks - PrevKey->Time;
                }
                
                ASSERT(NextKey && (TickDistance > 0.0f));
                
                // NOTE(Dima): Lerping
                f32 t = (CurTickTime - PrevKey->Time) / TickDistance;
                
                Result = Lerp(PrevKey->Value, NextKey->Value, t);
            }
            else{
                // NOTE(Dima): If last key frame and not looping
                animation_vector_key* PrevKey = &Keys[FoundPrevIndex];
                
                Result = PrevKey->Value;
            }
        }
    }
    
    return(Result);
}

INTERNAL_FUNCTION quat GetAnimatedQuat(animation_clip* Animation,
                                       animation_quaternion_key* Keys,
                                       int KeysCount,
                                       f32 CurTickTime,
                                       quat DefaultValue){
    quat Result = DefaultValue;
    
    // NOTE(Dima): Loading first frame's values
    if(KeysCount){
        Result = Keys[0].Value;
        
        // NOTE(Dima): Finding frame index before CurTickTime
        int FoundPrevIndex = FindPrevFrameIndexForQuatKey(Keys,
                                                          KeysCount,
                                                          CurTickTime);
        
        if(FoundPrevIndex != -1){
            if((FoundPrevIndex != KeysCount) || Animation->IsLooping)
            {
                // NOTE(Dima): If not last key frame
                animation_quaternion_key* PrevKey = &Keys[FoundPrevIndex];
                
                animation_quaternion_key* NextKey = 0;
                f32 TickDistance = 0.0f;
                
                // NOTE(Dima): If found frame is not last
                if(FoundPrevIndex != KeysCount)
                {
                    NextKey = &Keys[FoundPrevIndex + 1];
                    
                    TickDistance = NextKey->Time - PrevKey->Time;
                }
                
                // NOTE(Dima): If animation is looping and founf frame is last
                if(Animation->IsLooping && (FoundPrevIndex == KeysCount))
                {
                    NextKey = &Keys[0];
                    
                    TickDistance = Animation->DurationTicks - PrevKey->Time;
                }
                
                ASSERT(NextKey && (TickDistance > 0.0f));
                
                // NOTE(Dima): Lerping
                f32 t = (CurTickTime - PrevKey->Time) / TickDistance;
                
                Result = Lerp(PrevKey->Value, NextKey->Value, t);
            }
            else{
                // NOTE(Dima): If last key frame and not looping
                animation_quaternion_key* PrevKey = &Keys[FoundPrevIndex];
                
                Result = PrevKey->Value;
            }
        }
    }
    
    return(Result);
}

struct animated_node_pose{
    quat R;
    v3 P;
    v3 S;
    
    int NodeIndex;
};

struct animated_pose{
    animated_node_pose* NodesPoses;
    int NodeCount;
    
    u32 NodesHierarchyCheckSum;
};

void CalculateNodeTransforms(model_info* Model,
                             animated_pose* Pose)
{
    ASSERT(Pose->NodesHierarchyCheckSum == Model->NodesHierarchyCheckSum);
    ASSERT(Pose->NodeCount == Model->NodeCount);
    
    for(int NodeIndex = 0;
        NodeIndex < Pose->NodeCount;
        NodeIndex++)
    {
        animated_node_pose* NodePose = &Pose->NodePoses[NodeIndex];
        
        m44 ToParentTranMatrix = 
            ScalingMatrix(NodePose->S) * 
            RotationMatrix(NodePose->R) * 
            TranslationMatrix(NodePose->P);
    }
}

void AnimateModel(model_info* Model){
    if(Model->AnimationCount){
        animation_clip* Animation = PushOrLoadAnimation(Assets, 
                                                        Model->AnimationIDs[0],
                                                        ASSET_LOAD_IMMEDIATE);
        
        ASSERT(Animation);
        
        for(int NodeAnimIndex = 0;
            NodeAnimIndex < Animation->NodeAnimationsCount;
            NodeAnimIndex++)
        {
            u32 NodeAnimID = Animation->NodeAnimationIDs[NodeAnimIndex];
            
            node_animation* NodeAnim = PushOrLoadNodeAnim(Assets,
                                                          NodeAnimID,
                                                          ASSET_LOAD_IMMEDIATE);
            
            ASSERT(NodeAnim);
            
            f64 CurrentTime = 1.0f;
            
            f64 CurrentTick = CurrentTime * Animation->TicksPerSecond;
            
            if(Animation->IsLooping && (Animation->DurationTicks > 0.0f)){
                CurrentTick = fmod(CurrentTick, Animation->DurationTicks);
            }
            
            v3 AnimatedP = GetAnimatedVector(Animation,
                                             NodeAnim->PositionKeys,
                                             NodeAnim->PositionKeysCount,
                                             CurrentTick,
                                             V3(0.0f, 0.0f, 0.0f));
            
            quat AnimatedR = GetAnimatedQuat(Animation,
                                             NodeAnim->RotationKeys,
                                             NodeAnim->RotationKeysCount,
                                             CurrentTick,
                                             QuatI());
            
            v3 AnimatedS = GetAnimatedVector(Animation,
                                             NodeAnim->ScalingKeys,
                                             NodeAnim->ScalingKeysCount,
                                             CurrentTick,
                                             V3(1.0f, 1.0f, 1.0f));
            
            
        }
    }
}