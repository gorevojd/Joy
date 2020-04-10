#include "joy_animation.h"

INTERNAL_FUNCTION int FindPrevFrameIndexForKey(float* Times, 
                                               int KeysCount, 
                                               f32 CurTickTime)
{
    int Result = -1;
    
    for(int KeyIndex = 0;
        KeyIndex < KeysCount;
        KeyIndex++)
    {
        float Time = Times[KeyIndex];
        
        if(Time > CurTickTime){
            Result = KeyIndex - 1;
            
            break;
        }
    }
    
    if(Result == -1){
        Result = KeysCount - 1;
    }
    
    return(Result);
}

struct find_anim_deltas_ctx{
    int PrevKeyIndex;
    int NextKeyIndex;
    float t;
};

INTERNAL_FUNCTION find_anim_deltas_ctx
FindAnimDeltas(animation_clip* Animation,
               float* Times,
               int KeysCount,
               f32 CurTickTime)
{
    find_anim_deltas_ctx Result = {};
    
    // NOTE(Dima): Finding frame index before CurTickTime
    int FoundPrevIndex = FindPrevFrameIndexForKey(Times,
                                                  KeysCount,
                                                  CurTickTime);
    
    int LastFrameIndex = KeysCount - 1;
    
    // NOTE(Dima): If not last key frame
    float PrevKeyTime = Times[FoundPrevIndex];
    
    int NextKeyIndex;
    f32 TickDistance = 0.0f;
    
    // NOTE(Dima): If found frame is not last
    if(FoundPrevIndex != LastFrameIndex)
    {
        NextKeyIndex = FoundPrevIndex + 1;
        float NextKeyTime = Times[NextKeyIndex];
        
        TickDistance = NextKeyTime - PrevKeyTime;
    }
    else{
        if(Animation->IsLooping){
            NextKeyIndex = 0;
            
            TickDistance = Animation->DurationTicks - PrevKeyTime + 1.0f;
        }
        else{
            // NOTE(Dima): If animation is not looping - set next key to previous to make
            // NOTE(Dima): sure transform stays the same
            NextKeyIndex = FoundPrevIndex;
            
            TickDistance = 1.0f;
        }
    }
    
    // NOTE(Dima): Lerping
    f32 t = (CurTickTime - PrevKeyTime) / TickDistance;
    
    Result.PrevKeyIndex = FoundPrevIndex;
    Result.NextKeyIndex = NextKeyIndex;
    Result.t = t;
    
    return(Result);
}

INTERNAL_FUNCTION v3 GetAnimatedVector(animation_clip* Animation,
                                       v3* Values,
                                       float* Times,
                                       int KeysCount,
                                       f32 CurTickTime,
                                       v3 DefaultValue)
{
    v3 Result = DefaultValue;
    
    // NOTE(Dima): Loading first frame's values
    if(KeysCount){
        find_anim_deltas_ctx AnimDeltasCtx = FindAnimDeltas(Animation,
                                                            Times,
                                                            KeysCount,
                                                            CurTickTime);
        
        v3 PrevValue = Values[AnimDeltasCtx.PrevKeyIndex];
        v3 NextValue = Values[AnimDeltasCtx.NextKeyIndex];
        
        Result = Lerp(PrevValue, NextValue, AnimDeltasCtx.t);
    }
    
    return(Result);
}

INTERNAL_FUNCTION quat GetAnimatedQuat(animation_clip* Animation,
                                       quat* Values,
                                       float* Times,
                                       int KeysCount,
                                       f32 CurTickTime,
                                       quat DefaultValue){
    quat Result = DefaultValue;
    
    // NOTE(Dima): Loading first frame's values
    if(KeysCount){
        find_anim_deltas_ctx AnimDeltasCtx = FindAnimDeltas(Animation,
                                                            Times,
                                                            KeysCount,
                                                            CurTickTime);
        
        quat PrevValue = Values[AnimDeltasCtx.PrevKeyIndex];
        quat NextValue = Values[AnimDeltasCtx.NextKeyIndex];
        
        Result = Lerp(PrevValue, NextValue, AnimDeltasCtx.t);
    }
    
    return(Result);
}

INTERNAL_FUNCTION void 
DecomposeTransformsForNode(const m44& Matrix,
                           node_transform* Tran)
{
    Tran->T = Matrix.Rows[3].xyz;
    
    v3 Row0 = Matrix.Rows[0].xyz;
    v3 Row1 = Matrix.Rows[1].xyz;
    v3 Row2 = Matrix.Rows[2].xyz;
    
    float Row0Len = Magnitude(Row0);
    float Row1Len = Magnitude(Row1);
    float Row2Len = Magnitude(Row2);
    
    Tran->S = V3(Row0Len, Row1Len, Row2Len);
    
    Row0 = Row0 / Row0Len;
    Row1 = Row1 / Row1Len;
    Row2 = Row2 / Row2Len;
    
    m33 RotMat = MatrixFromRows(Row0, Row1, Row2);
    
    Tran->R = QuatFromM33(RotMat);
    
    Tran->Calculated = true;
}

void UpdateModelAnimation(assets* Assets,
                          model_info* Model,
                          playing_animation* PlayingAnim,
                          animation_clip* Animation,
                          f64 CurrentTime)
{
    // NOTE(Dima): Updating animation
    if(Animation){
        
        for(int NodeIndex = 0;
            NodeIndex < Model->NodeCount;
            NodeIndex++)
        {
            node_info* Node = &Model->Nodes[NodeIndex];
            
            node_transform* NodeTransform = &PlayingAnim->NodeTransforms[NodeIndex];
            
            NodeTransform->Calculated = false;
        }
        
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
                                             NodeAnim->PositionKeysValues,
                                             NodeAnim->PositionKeysTimes,
                                             NodeAnim->PositionKeysCount,
                                             CurrentTick,
                                             V3(0.0f, 0.0f, 0.0f));
            
            quat AnimatedR = GetAnimatedQuat(Animation,
                                             NodeAnim->RotationKeysValues,
                                             NodeAnim->RotationKeysTimes,
                                             NodeAnim->RotationKeysCount,
                                             CurrentTick,
                                             QuatI());
            
            v3 AnimatedS = GetAnimatedVector(Animation,
                                             NodeAnim->ScalingKeysValues,
                                             NodeAnim->ScalingKeysTimes,
                                             NodeAnim->ScalingKeysCount,
                                             CurrentTick,
                                             V3(1.0f, 1.0f, 1.0f));
            
            node_transform* NodeTransform = &PlayingAnim->NodeTransforms[NodeAnim->NodeIndex];
            
            NodeTransform->T = AnimatedP;
            NodeTransform->R = AnimatedR;
            NodeTransform->S = AnimatedS;
            NodeTransform->Calculated = true;
        }
        
        // NOTE(Dima): Extract transforms that were not calculated
        for(int NodeIndex = 0;
            NodeIndex < Model->NodeCount;
            NodeIndex++)
        {
            node_info* Node = &Model->Nodes[NodeIndex];
            
            node_transform* NodeTransform = &PlayingAnim->NodeTransforms[NodeIndex];
            
            if(!NodeTransform->Calculated){
                DecomposeTransformsForNode(Node->Shared->ToParent, NodeTransform);
            }
        }
    }
}


void CalculateToModelTransforms(model_info* Model){
    // NOTE(Dima): Calculating to to modelspace transforms
    for(int NodeIndex = 0;
        NodeIndex < Model->NodeCount;
        NodeIndex++)
    {
        node_info* Node = &Model->Nodes[NodeIndex];
        
        if(Node->Shared->ParentIndex != -1){
            // NOTE(Dima): If is not root
            node_info* ParentNode = &Model->Nodes[Node->Shared->ParentIndex];
            
            Node->CalculatedToModel = Node->CalculatedToParent* ParentNode->CalculatedToModel;
        }
        else{
            Node->CalculatedToModel = Node->CalculatedToParent;
        }
    }
}

// NOTE(Dima): Returns bone count
int UpdateModelBoneTransforms(model_info* Model, 
                              skeleton_info* Skeleton,
                              m44* BoneTransformMatrices)
{
    int BoneCount = 0;
    
    // NOTE(Dima): Updating skeleton
    if(Skeleton){
        BoneCount = Skeleton->BoneCount;
        
        for(int BoneIndex = 0;
            BoneIndex < BoneCount;
            BoneIndex++)
        {
            bone_info* Bone = &Skeleton->Bones[BoneIndex];
            
            node_info* CorrespondingNode = &Model->Nodes[Bone->NodeIndex];
            
            BoneTransformMatrices[BoneIndex] = 
                Bone->InvBindPose * CorrespondingNode->CalculatedToModel;
        }
    }
    
    return(BoneCount);
}