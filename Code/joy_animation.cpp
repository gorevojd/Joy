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
        
        if(Key->Time > CurTickTime){
            Result = KeyIndex - 1;
            
            break;
        }
    }
    
    if(Result == -1){
        Result = KeysCount - 1;
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
        
        if(Key->Time > CurTickTime){
            Result = KeyIndex - 1;
            
            break;
        }
    }
    
    if(Result == -1){
        Result = KeysCount - 1;
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
        
        int LastFrameIndex = KeysCount - 1;
        
        // NOTE(Dima): If not last key frame
        animation_vector_key* PrevKey = &Keys[FoundPrevIndex];
        
        animation_vector_key* NextKey = 0;
        f32 TickDistance = 0.0f;
        
        // NOTE(Dima): If found frame is not last
        if(FoundPrevIndex != LastFrameIndex)
        {
            NextKey = &Keys[FoundPrevIndex + 1];
            
            TickDistance = NextKey->Time - PrevKey->Time;
        }
        else{
            if(Animation->IsLooping){
                NextKey = &Keys[0];
                
                TickDistance = Animation->DurationTicks - PrevKey->Time + 1.0f;
                
                ASSERT(NextKey && (TickDistance > 0.0f));
            }
            else{
                // NOTE(Dima): If animation is not looping - set next key to previous to make
                // NOTE(Dima): sure transform stays the same
                NextKey = PrevKey;
                
                TickDistance = 1.0f;
            }
        }
        
        // NOTE(Dima): Lerping
        f32 t = (CurTickTime - PrevKey->Time) / TickDistance;
        
        Result = Lerp(PrevKey->Value, NextKey->Value, t);
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
        
        int LastFrameIndex = KeysCount - 1;
        
        // NOTE(Dima): If not last key frame
        animation_quaternion_key* PrevKey = &Keys[FoundPrevIndex];
        
        animation_quaternion_key* NextKey = 0;
        f32 TickDistance = 0.0f;
        
        // NOTE(Dima): If found frame is not last
        if(FoundPrevIndex != LastFrameIndex)
        {
            NextKey = &Keys[FoundPrevIndex + 1];
            
            TickDistance = NextKey->Time - PrevKey->Time;
        }
        else{
            if(Animation->IsLooping){
                NextKey = &Keys[0];
                
                TickDistance = Animation->DurationTicks - PrevKey->Time + 1.0f;
                
                ASSERT(NextKey && (TickDistance > 0.0f));
            }
            else{
                // NOTE(Dima): If animation is not looping - set next key to previous to make
                // NOTE(Dima): sure transform stays the same
                NextKey = PrevKey;
                
                TickDistance = 1.0f;
            }
        }
        
        // NOTE(Dima): Lerping
        f32 t = (CurTickTime - PrevKey->Time) / TickDistance;
        
        ASSERT(t >= 0 && t <= 1);
        
        Result = Lerp(PrevKey->Value, NextKey->Value, t);
    }
    
    return(Result);
}

void AnimateModel(assets* Assets,
                  model_info* Model, 
                  f64 CurrentTime)
{
    if(Model->AnimationCount){
        animation_clip* Animation = LoadAnimationClip(Assets, 
                                                      Model->AnimationIDs[0],
                                                      ASSET_IMPORT_IMMEDIATE);
        
        ASSERT(Animation);
        
        for(int NodeAnimIndex = 0;
            NodeAnimIndex < Animation->NodeAnimationsCount;
            NodeAnimIndex++)
        {
            u32 NodeAnimID = Animation->NodeAnimationIDs[NodeAnimIndex];
            
            node_animation* NodeAnim = LoadNodeAnim(Assets,
                                                    NodeAnimID,
                                                    ASSET_IMPORT_IMMEDIATE);
            
            ASSERT(NodeAnim);
            
            // NOTE(Dima): Animating
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
            
            m44 TraMatrix = TranslationMatrix(AnimatedP);
            m44 RotMatrix = RotationMatrix(AnimatedR);
            m44 ScaMatrix = ScalingMatrix(AnimatedS);
            
            m44 Tran = 
                ScalingMatrix(AnimatedS) * 
                RotationMatrix(AnimatedR) * 
                TranslationMatrix(AnimatedP);
            
            node_info* Node = &Model->Nodes[NodeAnim->NodeIndex];
            
            Node->CalculatedToParent = Tran;
        }
    }
}