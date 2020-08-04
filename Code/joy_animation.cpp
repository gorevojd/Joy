INTERNAL_FUNCTION inline int FindPrevFrameIndexForKey(float* Times, 
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

INTERNAL_FUNCTION inline int FindPrevFrameIndexForKeyBinary(float* Times, 
                                                            int KeysCount, 
                                                            f32 CurTickTime)
{
    int Result = -1;
    
    if((Times[KeysCount - 1] <= CurTickTime) || 
       (KeysCount == 1))
    {
        Result = KeysCount - 1;
    }
    else{
        int Start = 0;
        int OnePastLast = KeysCount;
        
        while((OnePastLast - Start) > 1){
            int MidIndex = (Start + OnePastLast) >> 1;
            
            if(Times[MidIndex] < CurTickTime){
                Start = MidIndex;
            }
            else{
                OnePastLast = MidIndex;
            }
        }
        
        Result = Start;
    }
    
    return(Result);
}

INTERNAL_FUNCTION inline find_anim_deltas_ctx
FindAnimDeltas(animation_clip* Animation,
               float* Times,
               int KeysCount,
               f32 CurTickTime,
               b32 IsLooping)
{
    find_anim_deltas_ctx Result = {};
    
    Result.Success = KeysCount > 0;
    if(Result.Success){
        // NOTE(Dima): Finding frame index before CurTickTime
        int FoundPrevIndex = 0;
        
#if 1
        if((int)Animation->DurationTicks == KeysCount - 1){
            FoundPrevIndex = (int)(CurTickTime);
        }
        else{
            FoundPrevIndex = FindPrevFrameIndexForKeyBinary(Times,
                                                            KeysCount,
                                                            CurTickTime);
        }
#else
        FoundPrevIndex = FindPrevFrameIndexForKeyBinary(Times,
                                                        KeysCount,
                                                        CurTickTime);
#endif
        
        int NextKeyIndex = 0;
        f32 TickDistance = 1.0f;
        f32 PrevKeyTime = 0.0f;
        int LastFrameIndex = KeysCount - 1;
        
        if(FoundPrevIndex >= LastFrameIndex)
        {
            FoundPrevIndex = LastFrameIndex;
            PrevKeyTime = Times[LastFrameIndex];
            if(IsLooping){
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
        else{
            PrevKeyTime = Times[FoundPrevIndex];
            NextKeyIndex = FoundPrevIndex + 1;
            TickDistance = Times[NextKeyIndex] - PrevKeyTime;
        }
        
        // NOTE(Dima): Lerping
        Result.PrevKeyIndex = FoundPrevIndex;
        Result.NextKeyIndex = NextKeyIndex;
        Result.t = (CurTickTime - PrevKeyTime) / TickDistance;
    }
    
    return(Result);
}

INTERNAL_FUNCTION inline v3 GetAnimatedVector(const find_anim_deltas_ctx& AnimDeltas,
                                              v3* Values,
                                              v3 DefaultValue)
{
    v3 Result = DefaultValue;
    
    // NOTE(Dima): Loading first frame's values
    if(AnimDeltas.Success){
        
        v3 PrevValue = Values[AnimDeltas.PrevKeyIndex];
        v3 NextValue = Values[AnimDeltas.NextKeyIndex];
        
        // NOTE(Dima): Lerping vectors
        float OneMinusT = 1.0f - AnimDeltas.t;
        Result = V3(PrevValue.x * OneMinusT + NextValue.x * AnimDeltas.t,
                    PrevValue.y * OneMinusT + NextValue.y * AnimDeltas.t,
                    PrevValue.z * OneMinusT + NextValue.z * AnimDeltas.t);
    }
    
    return(Result);
}

INTERNAL_FUNCTION inline quat GetAnimatedQuat(const find_anim_deltas_ctx& AnimDeltas,
                                              quat* Values,
                                              quat DefaultValue)
{
    quat Result = DefaultValue;
    
    // NOTE(Dima): Loading first frame's values
    if(AnimDeltas.Success){
        quat PrevValue = Values[AnimDeltas.PrevKeyIndex];
        quat NextValue = Values[AnimDeltas.NextKeyIndex];
        
        // NOTE(Dima): Lerping quaternion
        Result = Lerp(PrevValue, NextValue, AnimDeltas.t);
    }
    
    return(Result);
}

INTERNAL_FUNCTION node_transform DecomposeTransformsForNode(const m44& Matrix)
{
    node_transform Result;
    Result.T = Matrix.Rows[3].xyz;
    
    v3 Row0 = Matrix.Rows[0].xyz;
    v3 Row1 = Matrix.Rows[1].xyz;
    v3 Row2 = Matrix.Rows[2].xyz;
    
    float Row0Len = Magnitude(Row0);
    float Row1Len = Magnitude(Row1);
    float Row2Len = Magnitude(Row2);
    
    Result.S = V3(Row0Len, Row1Len, Row2Len);
    
    Row0 = Row0 / Row0Len;
    Row1 = Row1 / Row1Len;
    Row2 = Row2 / Row2Len;
    
    m33 RotMat = MatrixFromRows(Row0, Row1, Row2);
    
    Result.R = QuatFromM33(RotMat);
    
    return(Result);
}

INTERNAL_FUNCTION void ClearNodeTransforms(node_transforms_block* Transforms, int Count){
    FUNCTION_TIMING();
    
    v3 NullVector = V3(0.0f, 0.0f, 0.0f);
    quat NullQuat = Quat(0.0f, 0.0f, 0.0f, 0.0f);
    
#if !defined(JOY_AVX)    
    for(int NodeIndex = 0; 
        NodeIndex < Count; 
        NodeIndex++)
    {
        Transforms->Ts[NodeIndex] = NullVector;
        Transforms->Ss[NodeIndex] = NullVector;
        Transforms->Rs[NodeIndex] = NullQuat;
    }
#else
    
    v3_8x NullVector_4x = V3_8X(NullVector);
    v4_8x NullQuat_4x = V4_8X(NullQuat);
    
    for(int NodeIndex = 0; 
        NodeIndex < Count; 
        NodeIndex+=8)
    {
        V3_8X_Store(NullVector_4x, 
                    &Transforms->Ts[NodeIndex + 0],
                    &Transforms->Ts[NodeIndex + 1],
                    &Transforms->Ts[NodeIndex + 2],
                    &Transforms->Ts[NodeIndex + 3],
                    &Transforms->Ts[NodeIndex + 4],
                    &Transforms->Ts[NodeIndex + 5],
                    &Transforms->Ts[NodeIndex + 6],
                    &Transforms->Ts[NodeIndex + 7]);
        
        V3_8X_Store(NullVector_4x, 
                    &Transforms->Ss[NodeIndex + 0],
                    &Transforms->Ss[NodeIndex + 1],
                    &Transforms->Ss[NodeIndex + 2],
                    &Transforms->Ss[NodeIndex + 3],
                    &Transforms->Ss[NodeIndex + 4],
                    &Transforms->Ss[NodeIndex + 5],
                    &Transforms->Ss[NodeIndex + 6],
                    &Transforms->Ss[NodeIndex + 7]);
        
        V4_8X_Store(NullQuat_4x, 
                    &Transforms->Rs[NodeIndex + 0],
                    &Transforms->Rs[NodeIndex + 1],
                    &Transforms->Rs[NodeIndex + 2],
                    &Transforms->Rs[NodeIndex + 3],
                    &Transforms->Rs[NodeIndex + 4],
                    &Transforms->Rs[NodeIndex + 5],
                    &Transforms->Rs[NodeIndex + 6],
                    &Transforms->Rs[NodeIndex + 7]);
    }
#endif
    
}

struct update_pa_nodes_work{
    animation_clip* Animation;
    
    node_transforms_block Transforms;
    b32 Calculated[ANIM_TRANSFORMS_ARRAY_SIZE];
};

PLATFORM_CALLBACK(UpdatePANodesWork){
    update_pa_nodes_work* Work = (update_pa_nodes_work*)Data;
    
    
}

// TODO(Dima): Unroll deltas from here to make SOA
struct update_node_anim_data{
    node_animation* NodeAnim;
    find_anim_deltas_ctx PosDeltas;
    find_anim_deltas_ctx RotDeltas;
    find_anim_deltas_ctx ScaDeltas;
};

inline update_node_anim_data* GetNodeAnimData(update_node_anim_data* Datas,
                                              int Index, int Count,
                                              update_node_anim_data* Default)
{
    update_node_anim_data* Result = &Datas[Index];
    
    if(Index >= Count){
        Assert(Default);
        Result = Default;
    }
    
    return(Result);
}

enum update_pa_result_type{
    UpdatePA_Unknown,
    
    UpdatePA_ExitState,
    UpdatePA_StopPlayingAnimation,
    UpdatePA_PickNext,
    UpdatePA_PickRandom,
};

INTERNAL_FUNCTION u32 UpdatePlayingAnimation(asset_system* Assets,
                                             model_info* Model,
                                             playing_anim* Playing,
                                             animation_clip* Animation,
                                             f64 CurrentGlobalTime,
                                             f32 PlaybackRate)
{
    FUNCTION_TIMING();
    
    u32 Result = UpdatePA_Unknown;
    
    // NOTE(Dima): Updating animation
    if(Animation){
        for(int NodeIndex = 0;
            NodeIndex < Model->NodeCount;
            NodeIndex++)
        {
            Playing->TransformsCalculated[NodeIndex] = false;
        }
        
        b32 IsLooping = (Playing->ExitAction == AnimExitAction_Looping);
        
        // NOTE(Dima): Animating
        //f64 PrevTick = PreviousGlobalTime * Animation->TicksPerSecond;
        f64 PhaseTick = Playing->StartPhase01 * Animation->DurationTicks;
        f64 AnimTime = (CurrentGlobalTime - Playing->GlobalStart) * PlaybackRate;
        f64 CurrentTick = PhaseTick + AnimTime * Animation->TicksPerSecond;
        
        //Playing->Phase01 = 0.0f;
        
        if(CurrentTick >= Animation->DurationTicks){
            switch(Playing->ExitAction){
                case AnimExitAction_ExitState:{
                    Result = UpdatePA_ExitState;
                }break;
                
                case AnimExitAction_Stop:{
                    Result = UpdatePA_StopPlayingAnimation;
                }break;
                
                case AnimExitAction_Next:{
                    Result = UpdatePA_PickNext;
                }break;
                
                case AnimExitAction_Random:{
                    Result = UpdatePA_PickRandom;
                }break;
            }
        }
        
        if(Animation->DurationTicks > 0.0f){
            if(IsLooping){
                CurrentTick = fmod(CurrentTick, Animation->DurationTicks);
            }
            
            Playing->Phase01 = Clamp01(CurrentTick / Animation->DurationTicks);
        }
        
        update_node_anim_data NodeAnimationDatas[ANIM_TRANSFORMS_ARRAY_SIZE];
        int NumNodeAnimsToUpdate = 0;
        
        for(int NodeAnimIndex = 0;
            NodeAnimIndex < Animation->NodeAnimationsCount;
            NodeAnimIndex++)
        {
            u32 NodeAnimID = Animation->NodeAnimationIDs[NodeAnimIndex];
            
            node_animation* NodeAnim = LoadNodeAnim(Assets, NodeAnimID,
                                                    ASSET_IMPORT_IMMEDIATE);
            
            ASSERT(NodeAnim);
            
            NodeAnimationDatas[NumNodeAnimsToUpdate].NodeAnim = NodeAnim;
            NumNodeAnimsToUpdate++;
        }
        
        
        Playing->LastTick = Playing->NextTick;
        Playing->NextTick = CurrentTick;
        
        if(Playing->JustStarted && Animation->UsesRootMotion)
        {
            node_animation* RootNodeAnim = LoadNodeAnim(Assets, Animation->RootMotionNodeAnimID,
                                                        ASSET_IMPORT_IMMEDIATE);
            
            ASSERT(RootNodeAnim);
            
            
            find_anim_deltas_ctx PosDeltas = FindAnimDeltas(Animation,
                                                            RootNodeAnim->PositionKeysTimes,
                                                            RootNodeAnim->PositionKeysCount,
                                                            PhaseTick,
                                                            IsLooping);
            find_anim_deltas_ctx RotDeltas = FindAnimDeltas(Animation,
                                                            RootNodeAnim->RotationKeysTimes,
                                                            RootNodeAnim->RotationKeysCount,
                                                            PhaseTick,
                                                            IsLooping);
            find_anim_deltas_ctx ScaDeltas = FindAnimDeltas(Animation,
                                                            RootNodeAnim->ScalingKeysTimes,
                                                            RootNodeAnim->ScalingKeysCount,
                                                            PhaseTick,
                                                            IsLooping);
            
            v3 AnimatedP = GetAnimatedVector(PosDeltas,
                                             RootNodeAnim->PositionKeysValues,
                                             V3(0.0f, 0.0f, 0.0f));
            
            quat AnimatedR = GetAnimatedQuat(RotDeltas,
                                             RootNodeAnim->RotationKeysValues,
                                             QuatI());
            
            v3 AnimatedS = GetAnimatedVector(ScaDeltas,
                                             RootNodeAnim->ScalingKeysValues,
                                             V3(1.0f, 1.0f, 1.0f));
            
            Playing->RootLastP = AnimatedP;
            Playing->RootLastR = AnimatedR;
            Playing->RootLastS = AnimatedS;
            
            Playing->LastTick = PhaseTick;
            Playing->JustStarted = false;
        }
        
        {
            BLOCK_TIMING("UpdatePA::Finding deltas");
            
            for(int NodeAnimIndex = 0;
                NodeAnimIndex < NumNodeAnimsToUpdate;
                NodeAnimIndex++)
            {
                
                node_animation* NodeAnim = NodeAnimationDatas[NodeAnimIndex].NodeAnim;
                
                NodeAnimationDatas[NodeAnimIndex].PosDeltas = FindAnimDeltas(Animation,
                                                                             NodeAnim->PositionKeysTimes,
                                                                             NodeAnim->PositionKeysCount,
                                                                             CurrentTick,
                                                                             IsLooping);
                NodeAnimationDatas[NodeAnimIndex].RotDeltas = FindAnimDeltas(Animation,
                                                                             NodeAnim->RotationKeysTimes,
                                                                             NodeAnim->RotationKeysCount,
                                                                             CurrentTick,
                                                                             IsLooping);
                NodeAnimationDatas[NodeAnimIndex].ScaDeltas = FindAnimDeltas(Animation,
                                                                             NodeAnim->ScalingKeysTimes,
                                                                             NodeAnim->ScalingKeysCount,
                                                                             CurrentTick,
                                                                             IsLooping);
            }
        }
        
        {
            BLOCK_TIMING("UpdatePA::Lerping");
#if !defined(JOY_AVX)
            for(int NodeAnimIndex = 0;
                NodeAnimIndex < NumNodeAnimsToUpdate;
                NodeAnimIndex++)
            {
                
                update_node_anim_data* NodeAnimData = &NodeAnimationDatas[NodeAnimIndex];
                
                node_animation* NodeAnim = NodeAnimData->NodeAnim;
                
                v3 AnimatedP = GetAnimatedVector(NodeAnimData->PosDeltas,
                                                 NodeAnim->PositionKeysValues,
                                                 V3(0.0f, 0.0f, 0.0f));
                
                quat AnimatedR = GetAnimatedQuat(NodeAnimData->RotDeltas,
                                                 NodeAnim->RotationKeysValues,
                                                 QuatI());
                
                v3 AnimatedS = GetAnimatedVector(NodeAnimData->ScaDeltas,
                                                 NodeAnim->ScalingKeysValues,
                                                 V3(1.0f, 1.0f, 1.0f));
                
                int NodeIndex = NodeAnim->NodeIndex;
                Playing->NodeTransforms.Ts[NodeIndex] = AnimatedP;
                Playing->NodeTransforms.Rs[NodeIndex] = AnimatedR;
                Playing->NodeTransforms.Ss[NodeIndex] = AnimatedS;
                Playing->TransformsCalculated[NodeIndex] = true;
            }
#else
            v3_8x DefaultT = V3_8X(V3(0.0f));
            v4_8x DefaultR = V4_8X(QuatI());
            v3_8x DefaultS = V3_8X(V3(1.0f));
            
            int NodeAnimIndex = 0;
            for(NodeAnimIndex;
                NodeAnimIndex < NumNodeAnimsToUpdate;
                NodeAnimIndex += 8)
            {
                i32_8x Indices = IndicesStartFrom(NodeAnimIndex, 1);
                f32_8x IndicesFitInRange = LessThan(ConvertToFloat(Indices), ConvertToFloat(I32_8X(NumNodeAnimsToUpdate)));
                
                // NOTE(Dima): If at least one of node animations exist
                int MovedMask = AnyTrue(IndicesFitInRange);
                if(MovedMask){
                    update_node_anim_data* NodeAnimData0 = &NodeAnimationDatas[NodeAnimIndex + 0];
                    update_node_anim_data* NodeAnimData1 = GetNodeAnimData(NodeAnimationDatas, NodeAnimIndex + 1,
                                                                           NumNodeAnimsToUpdate,
                                                                           NodeAnimData0);
                    update_node_anim_data* NodeAnimData2 = GetNodeAnimData(NodeAnimationDatas, NodeAnimIndex + 2,
                                                                           NumNodeAnimsToUpdate, 
                                                                           NodeAnimData0);
                    update_node_anim_data* NodeAnimData3 = GetNodeAnimData(NodeAnimationDatas, NodeAnimIndex + 3,
                                                                           NumNodeAnimsToUpdate, 
                                                                           NodeAnimData0);
                    update_node_anim_data* NodeAnimData4 = GetNodeAnimData(NodeAnimationDatas, NodeAnimIndex + 4,
                                                                           NumNodeAnimsToUpdate, 
                                                                           NodeAnimData0);
                    update_node_anim_data* NodeAnimData5 = GetNodeAnimData(NodeAnimationDatas, NodeAnimIndex + 5,
                                                                           NumNodeAnimsToUpdate, 
                                                                           NodeAnimData0);
                    update_node_anim_data* NodeAnimData6 = GetNodeAnimData(NodeAnimationDatas, NodeAnimIndex + 6,
                                                                           NumNodeAnimsToUpdate, 
                                                                           NodeAnimData0);
                    update_node_anim_data* NodeAnimData7 = GetNodeAnimData(NodeAnimationDatas, NodeAnimIndex + 7,
                                                                           NumNodeAnimsToUpdate, 
                                                                           NodeAnimData0);
                    
                    node_animation* NodeAnim0 = NodeAnimData0->NodeAnim;
                    node_animation* NodeAnim1 = NodeAnimData1->NodeAnim;
                    node_animation* NodeAnim2 = NodeAnimData2->NodeAnim;
                    node_animation* NodeAnim3 = NodeAnimData3->NodeAnim;
                    node_animation* NodeAnim4 = NodeAnimData4->NodeAnim;
                    node_animation* NodeAnim5 = NodeAnimData5->NodeAnim;
                    node_animation* NodeAnim6 = NodeAnimData6->NodeAnim;
                    node_animation* NodeAnim7 = NodeAnimData7->NodeAnim;
                    
                    find_anim_deltas_ctx* PosDeltas0 = &NodeAnimData0->PosDeltas;
                    find_anim_deltas_ctx* PosDeltas1 = &NodeAnimData1->PosDeltas;
                    find_anim_deltas_ctx* PosDeltas2 = &NodeAnimData2->PosDeltas;
                    find_anim_deltas_ctx* PosDeltas3 = &NodeAnimData3->PosDeltas;
                    find_anim_deltas_ctx* PosDeltas4 = &NodeAnimData4->PosDeltas;
                    find_anim_deltas_ctx* PosDeltas5 = &NodeAnimData5->PosDeltas;
                    find_anim_deltas_ctx* PosDeltas6 = &NodeAnimData6->PosDeltas;
                    find_anim_deltas_ctx* PosDeltas7 = &NodeAnimData7->PosDeltas;
                    
                    find_anim_deltas_ctx* RotDeltas0 = &NodeAnimData0->RotDeltas;
                    find_anim_deltas_ctx* RotDeltas1 = &NodeAnimData1->RotDeltas;
                    find_anim_deltas_ctx* RotDeltas2 = &NodeAnimData2->RotDeltas;
                    find_anim_deltas_ctx* RotDeltas3 = &NodeAnimData3->RotDeltas;
                    find_anim_deltas_ctx* RotDeltas4 = &NodeAnimData4->RotDeltas;
                    find_anim_deltas_ctx* RotDeltas5 = &NodeAnimData5->RotDeltas;
                    find_anim_deltas_ctx* RotDeltas6 = &NodeAnimData6->RotDeltas;
                    find_anim_deltas_ctx* RotDeltas7 = &NodeAnimData7->RotDeltas;
                    
                    find_anim_deltas_ctx* ScaDeltas0 = &NodeAnimData0->ScaDeltas;
                    find_anim_deltas_ctx* ScaDeltas1 = &NodeAnimData1->ScaDeltas;
                    find_anim_deltas_ctx* ScaDeltas2 = &NodeAnimData2->ScaDeltas;
                    find_anim_deltas_ctx* ScaDeltas3 = &NodeAnimData3->ScaDeltas;
                    find_anim_deltas_ctx* ScaDeltas4 = &NodeAnimData4->ScaDeltas;
                    find_anim_deltas_ctx* ScaDeltas5 = &NodeAnimData5->ScaDeltas;
                    find_anim_deltas_ctx* ScaDeltas6 = &NodeAnimData6->ScaDeltas;
                    find_anim_deltas_ctx* ScaDeltas7 = &NodeAnimData7->ScaDeltas;
                    
                    // NOTE(Dima): Getting conditions
                    f32_8x SucceededT = IsTrue(I32_8X(PosDeltas0->Success,
                                                      PosDeltas1->Success,
                                                      PosDeltas2->Success,
                                                      PosDeltas3->Success,
                                                      PosDeltas4->Success,
                                                      PosDeltas5->Success,
                                                      PosDeltas6->Success,
                                                      PosDeltas7->Success));
                    
                    f32_8x SucceededR = IsTrue(I32_8X(RotDeltas0->Success,
                                                      RotDeltas1->Success,
                                                      RotDeltas2->Success,
                                                      RotDeltas3->Success,
                                                      RotDeltas4->Success,
                                                      RotDeltas5->Success,
                                                      RotDeltas6->Success,
                                                      RotDeltas7->Success));
                    
                    f32_8x SucceededS = IsTrue(I32_8X(ScaDeltas0->Success,
                                                      ScaDeltas1->Success,
                                                      ScaDeltas2->Success,
                                                      ScaDeltas3->Success,
                                                      ScaDeltas4->Success,
                                                      ScaDeltas5->Success,
                                                      ScaDeltas6->Success,
                                                      ScaDeltas7->Success));
                    
                    if(AnyTrue(SucceededT))
                    {
                        // NOTE(Dima): Setting position
                        v3_8x PrevValueT = V3_8X(NodeAnim0->PositionKeysValues[PosDeltas0->PrevKeyIndex],
                                                 NodeAnim1->PositionKeysValues[PosDeltas1->PrevKeyIndex],
                                                 NodeAnim2->PositionKeysValues[PosDeltas2->PrevKeyIndex],
                                                 NodeAnim3->PositionKeysValues[PosDeltas3->PrevKeyIndex],
                                                 NodeAnim4->PositionKeysValues[PosDeltas4->PrevKeyIndex],
                                                 NodeAnim5->PositionKeysValues[PosDeltas5->PrevKeyIndex],
                                                 NodeAnim6->PositionKeysValues[PosDeltas6->PrevKeyIndex],
                                                 NodeAnim7->PositionKeysValues[PosDeltas7->PrevKeyIndex]);
                        
                        v3_8x NextValueT = V3_8X(NodeAnim0->PositionKeysValues[PosDeltas0->NextKeyIndex],
                                                 NodeAnim1->PositionKeysValues[PosDeltas1->NextKeyIndex],
                                                 NodeAnim2->PositionKeysValues[PosDeltas2->NextKeyIndex],
                                                 NodeAnim3->PositionKeysValues[PosDeltas3->NextKeyIndex],
                                                 NodeAnim4->PositionKeysValues[PosDeltas4->NextKeyIndex],
                                                 NodeAnim5->PositionKeysValues[PosDeltas5->NextKeyIndex],
                                                 NodeAnim6->PositionKeysValues[PosDeltas6->NextKeyIndex],
                                                 NodeAnim7->PositionKeysValues[PosDeltas7->NextKeyIndex]);
                        
                        f32_8x TimeDeltaT = F32_8X(PosDeltas0->t,
                                                   PosDeltas1->t,
                                                   PosDeltas2->t,
                                                   PosDeltas3->t,
                                                   PosDeltas4->t,
                                                   PosDeltas5->t,
                                                   PosDeltas6->t,
                                                   PosDeltas7->t);
                        
                        // NOTE(Dima): Lerping
                        v3_8x LerpedT = Lerp(PrevValueT, NextValueT, TimeDeltaT);
                        LerpedT = ConditionalCombine(SucceededT, LerpedT, DefaultT);
                        
                        // NOTE(Dima): Storing values
                        v3* StoreToT[8] = {
                            &Playing->NodeTransforms.Ts[NodeAnim0->NodeIndex],
                            &Playing->NodeTransforms.Ts[NodeAnim1->NodeIndex],
                            &Playing->NodeTransforms.Ts[NodeAnim2->NodeIndex],
                            &Playing->NodeTransforms.Ts[NodeAnim3->NodeIndex],
                            &Playing->NodeTransforms.Ts[NodeAnim4->NodeIndex],
                            &Playing->NodeTransforms.Ts[NodeAnim5->NodeIndex],
                            &Playing->NodeTransforms.Ts[NodeAnim6->NodeIndex],
                            &Playing->NodeTransforms.Ts[NodeAnim7->NodeIndex]};
                        V3_8X_StoreConditional(LerpedT, MovedMask, StoreToT);
                    }
                    
                    // NOTE(Dima): Setting rotation
                    if(AnyTrue(SucceededR))
                    {
                        v4_8x PrevValueR = V4_8X(NodeAnim0->RotationKeysValues[RotDeltas0->PrevKeyIndex],
                                                 NodeAnim1->RotationKeysValues[RotDeltas1->PrevKeyIndex],
                                                 NodeAnim2->RotationKeysValues[RotDeltas2->PrevKeyIndex],
                                                 NodeAnim3->RotationKeysValues[RotDeltas3->PrevKeyIndex],
                                                 NodeAnim4->RotationKeysValues[RotDeltas4->PrevKeyIndex],
                                                 NodeAnim5->RotationKeysValues[RotDeltas5->PrevKeyIndex],
                                                 NodeAnim6->RotationKeysValues[RotDeltas6->PrevKeyIndex],
                                                 NodeAnim7->RotationKeysValues[RotDeltas7->PrevKeyIndex]);
                        
                        v4_8x NextValueR = V4_8X(NodeAnim0->RotationKeysValues[RotDeltas0->NextKeyIndex],
                                                 NodeAnim1->RotationKeysValues[RotDeltas1->NextKeyIndex],
                                                 NodeAnim2->RotationKeysValues[RotDeltas2->NextKeyIndex],
                                                 NodeAnim3->RotationKeysValues[RotDeltas3->NextKeyIndex],
                                                 NodeAnim4->RotationKeysValues[RotDeltas4->NextKeyIndex],
                                                 NodeAnim5->RotationKeysValues[RotDeltas5->NextKeyIndex],
                                                 NodeAnim6->RotationKeysValues[RotDeltas6->NextKeyIndex],
                                                 NodeAnim7->RotationKeysValues[RotDeltas7->NextKeyIndex]);
                        
                        f32_8x TimeDeltaR = F32_8X(RotDeltas0->t,
                                                   RotDeltas1->t,
                                                   RotDeltas2->t,
                                                   RotDeltas3->t,
                                                   RotDeltas4->t,
                                                   RotDeltas5->t,
                                                   RotDeltas6->t,
                                                   RotDeltas7->t);
                        
                        // NOTE(Dima): Lerping
                        v4_8x LerpedR = LerpQuat(PrevValueR, NextValueR, TimeDeltaR);
                        LerpedR = ConditionalCombine(SucceededR, LerpedR, DefaultR);
                        
                        // NOTE(Dima): Storing values
                        quat* StoreToR[8] = {
                            &Playing->NodeTransforms.Rs[NodeAnim0->NodeIndex],
                            &Playing->NodeTransforms.Rs[NodeAnim1->NodeIndex],
                            &Playing->NodeTransforms.Rs[NodeAnim2->NodeIndex],
                            &Playing->NodeTransforms.Rs[NodeAnim3->NodeIndex],
                            &Playing->NodeTransforms.Rs[NodeAnim4->NodeIndex],
                            &Playing->NodeTransforms.Rs[NodeAnim5->NodeIndex],
                            &Playing->NodeTransforms.Rs[NodeAnim6->NodeIndex],
                            &Playing->NodeTransforms.Rs[NodeAnim7->NodeIndex]};
                        V4_8X_StoreConditional(LerpedR, MovedMask, StoreToR);
                    }
                    
                    if(AnyTrue(SucceededS))
                    {
                        // NOTE(Dima): Setting scale
                        v3_8x PrevValueS = V3_8X(NodeAnim0->ScalingKeysValues[ScaDeltas0->PrevKeyIndex],
                                                 NodeAnim1->ScalingKeysValues[ScaDeltas1->PrevKeyIndex],
                                                 NodeAnim2->ScalingKeysValues[ScaDeltas2->PrevKeyIndex],
                                                 NodeAnim3->ScalingKeysValues[ScaDeltas3->PrevKeyIndex],
                                                 NodeAnim4->ScalingKeysValues[ScaDeltas4->PrevKeyIndex],
                                                 NodeAnim5->ScalingKeysValues[ScaDeltas5->PrevKeyIndex],
                                                 NodeAnim6->ScalingKeysValues[ScaDeltas6->PrevKeyIndex],
                                                 NodeAnim7->ScalingKeysValues[ScaDeltas7->PrevKeyIndex]);
                        
                        v3_8x NextValueS = V3_8X(NodeAnim0->ScalingKeysValues[ScaDeltas0->NextKeyIndex],
                                                 NodeAnim1->ScalingKeysValues[ScaDeltas1->NextKeyIndex],
                                                 NodeAnim2->ScalingKeysValues[ScaDeltas2->NextKeyIndex],
                                                 NodeAnim3->ScalingKeysValues[ScaDeltas3->NextKeyIndex],
                                                 NodeAnim4->ScalingKeysValues[ScaDeltas4->NextKeyIndex],
                                                 NodeAnim5->ScalingKeysValues[ScaDeltas5->NextKeyIndex],
                                                 NodeAnim6->ScalingKeysValues[ScaDeltas6->NextKeyIndex],
                                                 NodeAnim7->ScalingKeysValues[ScaDeltas7->NextKeyIndex]);
                        
                        f32_8x TimeDeltaS = F32_8X(ScaDeltas0->t,
                                                   ScaDeltas1->t,
                                                   ScaDeltas2->t,
                                                   ScaDeltas3->t,
                                                   ScaDeltas4->t,
                                                   ScaDeltas5->t,
                                                   ScaDeltas6->t,
                                                   ScaDeltas7->t);
                        
                        // NOTE(Dima): Lerping
                        v3_8x LerpedS = Lerp(PrevValueS, NextValueS, TimeDeltaS);
                        LerpedS = ConditionalCombine(SucceededS, LerpedS, DefaultS);
                        
                        v3* StoreToS[8] = {
                            &Playing->NodeTransforms.Ss[NodeAnim0->NodeIndex],
                            &Playing->NodeTransforms.Ss[NodeAnim1->NodeIndex],
                            &Playing->NodeTransforms.Ss[NodeAnim2->NodeIndex],
                            &Playing->NodeTransforms.Ss[NodeAnim3->NodeIndex],
                            &Playing->NodeTransforms.Ss[NodeAnim4->NodeIndex],
                            &Playing->NodeTransforms.Ss[NodeAnim5->NodeIndex],
                            &Playing->NodeTransforms.Ss[NodeAnim6->NodeIndex],
                            &Playing->NodeTransforms.Ss[NodeAnim7->NodeIndex]};
                        V3_8X_StoreConditional(LerpedS, MovedMask, StoreToS);
                    }
                    
                    b32* StoreToCalc[8] = {
                        &Playing->TransformsCalculated[NodeAnim0->NodeIndex],
                        &Playing->TransformsCalculated[NodeAnim1->NodeIndex],
                        &Playing->TransformsCalculated[NodeAnim2->NodeIndex],
                        &Playing->TransformsCalculated[NodeAnim3->NodeIndex],
                        &Playing->TransformsCalculated[NodeAnim4->NodeIndex],
                        &Playing->TransformsCalculated[NodeAnim5->NodeIndex],
                        &Playing->TransformsCalculated[NodeAnim6->NodeIndex],
                        &Playing->TransformsCalculated[NodeAnim7->NodeIndex]};
                    
                    I32_8X_StoreConditional(CastToInt(IndicesFitInRange), MovedMask, StoreToCalc);
                }
                else{
                    break;
                }
            }
#endif
        }
        
        {
            BLOCK_TIMING("UpdatePA::Decomposing");
            // NOTE(Dima): Extract transforms that were not calculated
            for(int NodeIndex = 0;
                NodeIndex < Model->NodeCount;
                NodeIndex++)
            {
                node_info* Node = &Model->Nodes[NodeIndex];
                
                if(Playing->TransformsCalculated[NodeIndex] == false){
                    node_transform Decomposed = DecomposeTransformsForNode(Node->Shared->ToParent);
                    
                    Playing->NodeTransforms.Ts[NodeIndex] = Decomposed.T;
                    Playing->NodeTransforms.Rs[NodeIndex] = Decomposed.R;
                    Playing->NodeTransforms.Ss[NodeIndex] = Decomposed.S;
                }
            }
        }
        
        
        Playing->AdvancedP = V3_Zero();
        Playing->AdvancedS = V3_Zero();
        Playing->AdvancedR = IdentityQuaternion();
        
        if(Animation->UsesRootMotion){
            
            node_animation* RootNodeAnim = LoadNodeAnim(Assets, Animation->RootMotionNodeAnimID,
                                                        ASSET_IMPORT_IMMEDIATE);
            
            ASSERT(RootNodeAnim);
            
            
            find_anim_deltas_ctx PosDeltas = FindAnimDeltas(Animation,
                                                            RootNodeAnim->PositionKeysTimes,
                                                            RootNodeAnim->PositionKeysCount,
                                                            CurrentTick,
                                                            IsLooping);
            find_anim_deltas_ctx RotDeltas = FindAnimDeltas(Animation,
                                                            RootNodeAnim->RotationKeysTimes,
                                                            RootNodeAnim->RotationKeysCount,
                                                            CurrentTick,
                                                            IsLooping);
            find_anim_deltas_ctx ScaDeltas = FindAnimDeltas(Animation,
                                                            RootNodeAnim->ScalingKeysTimes,
                                                            RootNodeAnim->ScalingKeysCount,
                                                            CurrentTick,
                                                            IsLooping);
            
            v3 RootCurP = GetAnimatedVector(PosDeltas,
                                            RootNodeAnim->PositionKeysValues,
                                            V3(0.0f, 0.0f, 0.0f));
            
            quat RootCurR = GetAnimatedQuat(RotDeltas,
                                            RootNodeAnim->RotationKeysValues,
                                            QuatI());
            
            v3 RootCurS = GetAnimatedVector(ScaDeltas,
                                            RootNodeAnim->ScalingKeysValues,
                                            V3(1.0f, 1.0f, 1.0f));
            
            if(Playing->NextTick >= Playing->LastTick)
            {
                Playing->AdvancedP = RootCurP - Playing->RootLastP;
                Playing->AdvancedS = RootCurS - Playing->RootLastS;
                Playing->AdvancedR = RotationDifference(Playing->RootLastR, RootCurR);
            }
            else
            {
                Playing->AdvancedP = (RootNodeAnim->EndP - Playing->RootLastP) + (RootCurP - RootNodeAnim->BeginP);
                Playing->AdvancedS = (RootNodeAnim->EndS - Playing->RootLastS) + (RootCurS - RootNodeAnim->BeginS);
                
                quat EndDiff = RotationDifference(Playing->RootLastR, RootNodeAnim->EndR);
                quat BeginDiff = RotationDifference(RootNodeAnim->BeginR, RootCurR);
                Playing->AdvancedR = BeginDiff * EndDiff;
            }
            
            Playing->RootLastP = RootCurP;
            Playing->RootLastR = RootCurR;
            Playing->RootLastS = RootCurS;
        }
    }
    
    return(Result);
}

// NOTE(Dima): Calculating initial node to parent transforms
INTERNAL_FUNCTION void ResetToParentTransforms(model_info* Model){
    for(int NodeIndex = 0;
        NodeIndex < Model->NodeCount;
        NodeIndex++)
    {
        node_info* Node = &Model->Nodes[NodeIndex];
        
        Node->CalculatedToParent = Node->Shared->ToParent;
    }
}

// NOTE(Dima): Calculating nodes to parent matrices based on transforms
INTERNAL_FUNCTION void CalculateToParentTransforms(model_info* Model, 
                                                   node_transforms_block* Transforms)
{
    FUNCTION_TIMING();
    
#if !defined(JOY_AVX)
    for(int NodeIndex = 0; 
        NodeIndex < Model->NodeCount;
        NodeIndex++)
    {
        node_info* TargetNode = &Model->Nodes[NodeIndex];
        
        // NOTE(Dima): Calculating to parent transform
        MulRefsToRef(TargetNode->CalculatedToParent,
                     MulRefs(ScalingMatrix(Transforms->Ss[NodeIndex]),
                             RotationMatrix(Transforms->Rs[NodeIndex])), 
                     TranslationMatrix(Transforms->Ts[NodeIndex]));
    }
#else
    int NodeIndex;
    for(NodeIndex = 0; 
        NodeIndex < Model->NodeCount;
        NodeIndex+=8)
    {
        i32_8x Indices = IndicesStartFrom(NodeIndex);
        f32_8x IndicesFit = LessThan(ConvertToFloat(Indices), ConvertToFloat(I32_8X(Model->NodeCount)));
        i32_8x StepIndices = Blend(IndicesFit, Indices, I32_8X(Model->NodeCount - 1));
        
        m44_8x Tra = TranslationMatrix(Transforms->Ts[MMI8(StepIndices.e, 0)],
                                       Transforms->Ts[MMI8(StepIndices.e, 1)],
                                       Transforms->Ts[MMI8(StepIndices.e, 2)],
                                       Transforms->Ts[MMI8(StepIndices.e, 3)],
                                       Transforms->Ts[MMI8(StepIndices.e, 4)],
                                       Transforms->Ts[MMI8(StepIndices.e, 5)],
                                       Transforms->Ts[MMI8(StepIndices.e, 6)],
                                       Transforms->Ts[MMI8(StepIndices.e, 7)]);
        
        m44_8x Rot = RotationMatrix(Transforms->Rs[MMI8(StepIndices.e, 0)],
                                    Transforms->Rs[MMI8(StepIndices.e, 1)],
                                    Transforms->Rs[MMI8(StepIndices.e, 2)],
                                    Transforms->Rs[MMI8(StepIndices.e, 3)],
                                    Transforms->Rs[MMI8(StepIndices.e, 4)],
                                    Transforms->Rs[MMI8(StepIndices.e, 5)],
                                    Transforms->Rs[MMI8(StepIndices.e, 6)],
                                    Transforms->Rs[MMI8(StepIndices.e, 7)]);
        
        m44_8x Sca = ScalingMatrix(Transforms->Ss[MMI8(StepIndices.e, 0)],
                                   Transforms->Ss[MMI8(StepIndices.e, 1)],
                                   Transforms->Ss[MMI8(StepIndices.e, 2)],
                                   Transforms->Ss[MMI8(StepIndices.e, 3)],
                                   Transforms->Ss[MMI8(StepIndices.e, 4)],
                                   Transforms->Ss[MMI8(StepIndices.e, 5)],
                                   Transforms->Ss[MMI8(StepIndices.e, 6)],
                                   Transforms->Ss[MMI8(StepIndices.e, 7)]);
        
        m44_8x Res = Sca * Rot * Tra;
        
        M44_8X_Store(Res, 
                     &Model->Nodes[MMI8(StepIndices.e, 0)].CalculatedToParent,
                     &Model->Nodes[MMI8(StepIndices.e, 1)].CalculatedToParent,
                     &Model->Nodes[MMI8(StepIndices.e, 2)].CalculatedToParent,
                     &Model->Nodes[MMI8(StepIndices.e, 3)].CalculatedToParent,
                     &Model->Nodes[MMI8(StepIndices.e, 4)].CalculatedToParent,
                     &Model->Nodes[MMI8(StepIndices.e, 5)].CalculatedToParent,
                     &Model->Nodes[MMI8(StepIndices.e, 6)].CalculatedToParent,
                     &Model->Nodes[MMI8(StepIndices.e, 7)].CalculatedToParent);
    }
#endif
}

INTERNAL_FUNCTION void CalculateToModelTransforms(model_info* Model){
    FUNCTION_TIMING();
    
    // NOTE(Dima): Calculating to to modelspace transforms
    
#if !defined(JOY_AVX)
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
#else
    i32_8x MinusOne = I32_8X(-1);
    
    for(int NodeIndex = 0;
        NodeIndex < Model->NodeCount;
        NodeIndex += 8)
    {
        i32_8x Indices = IndicesStartFrom(NodeIndex, 1);
        f32_8x IndicesFit = LessThan(ConvertToFloat(Indices), ConvertToFloat(I32_8X(Model->NodeCount)));
        i32_8x StepIndices = Blend(IndicesFit, Indices, I32_8X(Model->NodeCount - 1));
        
        node_info* Node0 = &Model->Nodes[MMI8(StepIndices.e, 0)];
        node_info* Node1 = &Model->Nodes[MMI8(StepIndices.e, 1)];
        node_info* Node2 = &Model->Nodes[MMI8(StepIndices.e, 2)];
        node_info* Node3 = &Model->Nodes[MMI8(StepIndices.e, 3)];
        node_info* Node4 = &Model->Nodes[MMI8(StepIndices.e, 4)];
        node_info* Node5 = &Model->Nodes[MMI8(StepIndices.e, 5)];
        node_info* Node6 = &Model->Nodes[MMI8(StepIndices.e, 6)];
        node_info* Node7 = &Model->Nodes[MMI8(StepIndices.e, 7)];
        
        node_info* ParentNode0 = &Model->Nodes[Node0->Shared->ParentIndex];
        node_info* ParentNode1 = &Model->Nodes[Node1->Shared->ParentIndex];
        node_info* ParentNode2 = &Model->Nodes[Node2->Shared->ParentIndex];
        node_info* ParentNode3 = &Model->Nodes[Node3->Shared->ParentIndex];
        node_info* ParentNode4 = &Model->Nodes[Node4->Shared->ParentIndex];
        node_info* ParentNode5 = &Model->Nodes[Node5->Shared->ParentIndex];
        node_info* ParentNode6 = &Model->Nodes[Node6->Shared->ParentIndex];
        node_info* ParentNode7 = &Model->Nodes[Node7->Shared->ParentIndex];
        
        m44_8x CalcToParent = M44_8X(Node0->CalculatedToParent,
                                     Node1->CalculatedToParent,
                                     Node2->CalculatedToParent,
                                     Node3->CalculatedToParent,
                                     Node4->CalculatedToParent,
                                     Node5->CalculatedToParent,
                                     Node6->CalculatedToParent,
                                     Node7->CalculatedToParent);
        
        m44_8x ParentToModel = M44_8X(ParentNode0->CalculatedToModel,
                                      ParentNode1->CalculatedToModel,
                                      ParentNode2->CalculatedToModel,
                                      ParentNode3->CalculatedToModel,
                                      ParentNode4->CalculatedToModel,
                                      ParentNode5->CalculatedToModel,
                                      ParentNode6->CalculatedToModel,
                                      ParentNode7->CalculatedToModel);
        
        m44_8x Result = CalcToParent * ParentToModel;
        
        
        f32_8x ParentIndexCheckMask = IsEqual(MinusOne, I32_8X(Node0->Shared->ParentIndex,
                                                               Node1->Shared->ParentIndex,
                                                               Node2->Shared->ParentIndex,
                                                               Node3->Shared->ParentIndex,
                                                               Node4->Shared->ParentIndex,
                                                               Node5->Shared->ParentIndex,
                                                               Node6->Shared->ParentIndex,
                                                               Node7->Shared->ParentIndex));
        
        if(AnyTrue(ParentIndexCheckMask)){
            Result = ConditionalCombine(ParentIndexCheckMask,
                                        CalcToParent, Result);
        }
        
        M44_8X_Store(Result,
                     &Node0->CalculatedToModel,
                     &Node1->CalculatedToModel,
                     &Node2->CalculatedToModel,
                     &Node3->CalculatedToModel,
                     &Node4->CalculatedToModel,
                     &Node5->CalculatedToModel,
                     &Node6->CalculatedToModel,
                     &Node7->CalculatedToModel);
    }
#endif
}

INTERNAL_FUNCTION inline void PlayAnimationInternal(playing_state_slot* Slot,
                                                    int AnimationIndex,
                                                    f64 GlobalStart,
                                                    f32 Phase)
{
    playing_anim* PlayAnim = Slot->PlayingAnimations[AnimationIndex];
    
    PlayAnim->GlobalStart = GlobalStart;
    PlayAnim->Phase01 = Phase;
    PlayAnim->StartPhase01 = Phase;
    PlayAnim->JustStarted = true;
}

void PlayStateAnimations(playing_state_slot* Slot, 
                         f64 GlobalStart, 
                         f32 Phase)
{
    anim_state* State = Slot->State;
    
    for(int Index = State->FirstAnimIndex, Temp = 0;
        Index < State->OnePastLastAnim;
        Index++, Temp++)
    {
        Slot->PlayingAnimations[Temp] = &Slot->AC->Anims[Index];
        
        PlayAnimationInternal(Slot, Temp, GlobalStart, Phase);
    }
    
    Slot->PlayingAnimationsCount = State->OnePastLastAnim - State->FirstAnimIndex;
}

b32 StateIsPlaying(animated_component* AC,
                   char* StateName)
{
    b32 Result = false;
    
    //for(int SlotIndex = 0; SlotIndex < ANIM_MAX_PLAYING_STATES; SlotIndex++){
    //playing_state_slot* Slot = &AC->PlayingStates[SlotIndex];
    playing_state_slot* Slot = &AC->PlayingStates[AC->PlayingIndex];
    
    anim_state* State = Slot->State;
    if(State && StringsAreEqual(State->Name, StateName)){
        Result = true;
        //break;
    }
    //}
    
    return(Result);
}

f32 GetPlayingStatePhase(animated_component* AC)
{
    playing_state_slot* Slot = &AC->PlayingStates[AC->PlayingIndex];
    
    f32 Result = Slot->PlayingAnimations[Slot->PlayingAnimIndex]->Phase01;
    
    return(Result);
}

INTERNAL_FUNCTION void TransitionToStateInternal(animated_component* AC,
                                                 anim_state* ToState,
                                                 f32 TimeToTransit,
                                                 f64 GlobalTime,
                                                 f32 Phase)
{
    int NextPlayIndex = GetNextPlayingIndex(AC->PlayingIndex);
    AC->PlayingStates[NextPlayIndex].State = ToState;
    AC->PlayingIndex = NextPlayIndex;
    AC->PlayingStatesCount++;
    
    playing_state_slot* NextSlot = &AC->PlayingStates[NextPlayIndex];
    
    NextSlot->TimeToTransit = TimeToTransit;
    NextSlot->TransitionTimeLeft = TimeToTransit;
    
    // NOTE(Dima): Init playing animations
    NextSlot->PlayingAnimationsCount = 0;
    NextSlot->PlayingAnimIndex = 0;
    for(int AnimIndex = 0;
        AnimIndex < ANIM_MAX_ANIMATIONS_PER_STATE;
        AnimIndex++)
    {
        NextSlot->PlayingAnimations[AnimIndex] = 0;
    }
    
    // NOTE(Dima): Play animations
    PlayStateAnimations(NextSlot, GlobalTime, 0.0f);
}

void ForceTransitionRequest(animated_component* AC,
                            char* StateName,
                            f32 TimeToTransit,
                            f32 Phase)
{
    
    anim_transition_request* Request = &AC->ForceTransitionRequest;
    
    Request->ToState = FindState(AC->Control, StateName);
    Assert(Request->ToState);
    Request->Requested = true;
    Request->TimeToTransit = TimeToTransit;
    Request->Phase = Phase;
}

INTERNAL_FUNCTION void ProcessInitTransitioning(animated_component* AC,
                                                f64 GlobalTime)
{
    
    playing_state_slot* Slot = &AC->PlayingStates[AC->PlayingIndex];
    
    anim_state* MainState = Slot->State;
    
    // NOTE(Dima): Iterating through all transitions
    b32 TransitionHappened = false;
    anim_transition* TransitionAt = MainState->FirstTransition;
    while(TransitionAt != 0){
        
        // NOTE(Dima): From state of transition should be equal to current state
        Assert(TransitionAt->FromState == MainState);
        
        b32 AllConditionsTrue = true;
        
        // NOTE(Dima): Checking conditions loop
        for(int ConditionIndex = 0;
            ConditionIndex < TransitionAt->ConditionsCount;
            ConditionIndex++)
        {
            anim_transition_condition* Cond = &TransitionAt->Conditions[ConditionIndex];
            
            anim_variable* Variable = FindVariable(AC, Cond->Name);
            Assert(Cond->VariableValueType == Variable->ValueType);
            
            b32 ConditionTrue = 0;
            if(Cond->VariableValueType == AnimVariable_Bool){
                b32 VariableBool = Variable->Value.Bool;
                b32 CondBool = Cond->Value.Bool;
                
                if(Cond->ConditionType == TransitionCondition_Equal){
                    ConditionTrue = (VariableBool == CondBool);
                }
            }
            else if(Cond->VariableValueType  == AnimVariable_Float){
                float VariableFloat = Variable->Value.Float;
                float CondFloat = Cond->Value.Float;
                
                switch(Cond->ConditionType){
                    case TransitionCondition_MoreEqThan:{
                        ConditionTrue = VariableFloat >= CondFloat;
                    }break;
                    
                    case TransitionCondition_MoreThan:{
                        ConditionTrue = VariableFloat > CondFloat;
                    }break;
                    
                    case TransitionCondition_LessEqThan:{
                        ConditionTrue = VariableFloat <= CondFloat;
                    }break;
                    
                    case TransitionCondition_LessThan:{
                        ConditionTrue = VariableFloat < CondFloat;
                    }break;
                    
                    case TransitionCondition_Equal:{
                        ConditionTrue = Abs(VariableFloat - CondFloat) < 0.00000001f;
                    }break;
                }
            }
            
            if(!ConditionTrue){
                AllConditionsTrue = false;
                break;
            }
        }
        
        b32 StateCanTransition = false;
        if(TransitionAt->AnimationShouldFinish){
            playing_anim* PlayAnim = Slot->PlayingAnimations[Slot->PlayingAnimIndex];
            StateCanTransition = ((Slot->StateShouldBeExited) || 
                                  (PlayAnim->Phase01 > (TransitionAt->TransitStartPhase - 0.00001f)));
            
            if(TransitionAt->ConditionsCount){
                StateCanTransition &= AllConditionsTrue;
            }
        }
        else{
            StateCanTransition = TransitionAt->ConditionsCount && AllConditionsTrue;
        }
        
        if(StateCanTransition){
            TransitionToStateInternal(AC, TransitionAt->ToState,
                                      TransitionAt->TimeToTransit,
                                      GlobalTime, 0.0f);
            
            TransitionHappened = true;
            
            break;
        }
        
        // NOTE(Dima): Advancing iterator
        TransitionAt = TransitionAt->NextInList;
    } // end loop through all transitions
    
    if(AC->ForceTransitionRequest.Requested && 
       AC->ForceTransitionRequest.ToState)
    {
        anim_transition_request* Request = &AC->ForceTransitionRequest;
        
        TransitionToStateInternal(AC, 
                                  Request->ToState,
                                  Request->TimeToTransit,
                                  GlobalTime, 
                                  Request->Phase);
        
        Request->Requested = false;
    }
    
#if 0
    if(AC->TryTransitionRequest.Requested &&
       AC->TryTransitionRequest.ToState &&
       !TransitionHappened)
    {
        anim_transition_request* Request = &AC->TryTransitionRequest;
        
        TransitionToStateInternal(AC, 
                                  Request->ToState,
                                  Request->TimeToTransit,
                                  GlobalTime, 
                                  Request->Phase);
        
        Request->Requested = false;
    }
#endif
}


/*
// NOTE(Dima): This funciton chooses which animations of playing state slot 
we should update.
*/
INTERNAL_FUNCTION playing_anim_indices 
GetUpdateIndices(playing_state_slot* Slot)
{
    anim_state* State = Slot->State;
    
    int* Indices = Slot->PlayingAnimIndices;
    int Count = 0;
    
    switch(State->Type){
        case AnimState_Animation:
        case AnimState_Queue:{
            Indices[0] = Slot->PlayingAnimIndex;
            Count = 1;
        }break;
        
        case AnimState_BlendTree:{
            // TODO(Dima): More smart if contribution is 0 for nodes of blend tree
            for(int AnimIndex = 0; 
                AnimIndex < Slot->PlayingAnimationsCount;
                AnimIndex++)
            {
                Indices[AnimIndex] = AnimIndex;
            }
            Count = Slot->PlayingAnimationsCount;
        }break;
        
    }
    
    playing_anim_indices Result = {};
    
    Result.Indices = Indices;
    Result.Count = Count;
    
    return(Result);
}

INTERNAL_FUNCTION void UpdateAnimatedComponent(asset_system* Assets,
                                               model_info* Model, 
                                               animated_component* AC,
                                               f64 GlobalTime,
                                               f32 DeltaTime,
                                               f32 PlaybackRate)
{
    FUNCTION_TIMING();
    
    if(AC){
        anim_controller* Control = AC->Control;
        
        ProcessInitTransitioning(AC, GlobalTime);
        
        // NOTE(Dima): Updating playing states, conditions, transitions
        if(AC->PlayingStatesCount > 0){
            // NOTE(Dima): Updating transition
            if(AC->PlayingStatesCount > 1){
                float TransitionPercentageLeft = 1.0f;
                b32 ShouldBreakTransitions = false;
                int TransitionsCount = AC->PlayingStatesCount - 1;
                for(int TransitionIndex = 0;
                    TransitionIndex < TransitionsCount;
                    TransitionIndex++)
                {
                    int CurrPlayIndex = GetModulatedPlayintIndex(AC->PlayingIndex - TransitionIndex);
                    int PrevPlayIndex = GetModulatedPlayintIndex(AC->PlayingIndex - TransitionIndex - 1);
                    
                    playing_state_slot* CurrSlot = &AC->PlayingStates[CurrPlayIndex];
                    playing_state_slot* PrevSlot = &AC->PlayingStates[PrevPlayIndex];
                    
                    if(!ShouldBreakTransitions){
                        // NOTE(Dima): If transitioning
                        b32 ShouldEndTransition = false;
                        
                        CurrSlot->TransitionTimeLeft -= DeltaTime * PlaybackRate;
                        if(CurrSlot->TimeToTransit > 0.0f){
                            if(CurrSlot->TransitionTimeLeft < 0.0000001f){
                                CurrSlot->TransitionTimeLeft = 0.0f;
                                ShouldEndTransition = true;
                            }
                            
                            f32 Weight0 = Clamp01(CurrSlot->TransitionTimeLeft / CurrSlot->TimeToTransit);
                            f32 Weight1 = 1.0f - Weight0;
                            
                            AC->PlayingStates[CurrPlayIndex].Contribution = Weight1 * TransitionPercentageLeft;
                            
                            if(TransitionIndex == (TransitionsCount - 1)){
                                AC->PlayingStates[PrevPlayIndex].Contribution = Weight0 * TransitionPercentageLeft;
                            }
                            else{
                                TransitionPercentageLeft = Weight0 * TransitionPercentageLeft;
                            }
                        }
                        else{
                            // NOTE(Dima): 0 here because this animation ended
                            AC->PlayingStates[PrevPlayIndex].Contribution = 0.0f;
                            
                            // NOTE(Dima): 1 here because this animation is fully turned on
                            AC->PlayingStates[CurrPlayIndex].Contribution = 1.0f * TransitionPercentageLeft;
                            
                            ShouldEndTransition = true;
                        }
                        
                        if(ShouldEndTransition){
                            AC->PlayingStatesCount = TransitionIndex + 1;
                            
                            CurrSlot->TimeToTransit = 0.0f;
                            CurrSlot->TransitionTimeLeft = 0.0f;
                            
                            PrevSlot->State = 0;
                            
                            ShouldBreakTransitions = true;
                        }
                    }
                    else{
                        CurrSlot->State = 0;
                        PrevSlot->State = 0;
                    }
                }
            }
            else if(AC->PlayingStatesCount == 1){
                AC->PlayingStates[AC->PlayingIndex].Contribution = 1.0f;
            }
            
            // NOTE(Dima): Setting states animations based on StateName->AnimID mapping
            for(int PlayingSlotIndex = 0;
                PlayingSlotIndex < ANIM_MAX_PLAYING_STATES;
                PlayingSlotIndex++)
            {
                playing_state_slot* Slot = &AC->PlayingStates[PlayingSlotIndex];
                anim_state* State = Slot->State;
                for(int AnimIndex = 0; 
                    AnimIndex < Slot->PlayingAnimationsCount;
                    AnimIndex++)
                {
                    playing_anim* PlayAnim = Slot->PlayingAnimations[AnimIndex];
                    
                    anim_animid* AnimID = FindAnimID(AC, PlayAnim->Name);
                    
                    Assert(AnimID);
                    
                    PlayAnim->AnimationID = AnimID->AnimID;
                }
            }
            
            // NOTE(Dima): Update animations and blend trees of all playing graph nodes
            {
                BLOCK_TIMING("UpdateAC::UpdateAllAnims");
                
                int PlayingStateIndex = AC->PlayingIndex;
                for(int Index  = 0;
                    Index < AC->PlayingStatesCount;
                    Index++)
                {
                    playing_state_slot* Slot = &AC->PlayingStates[PlayingStateIndex];
                    
                    playing_anim_indices IndicesToUpdate = GetUpdateIndices(Slot);
                    
                    anim_state* AnimState = Slot->State;
                    
                    b32 IsQueueOrSingle = ((AnimState->Type == AnimState_Queue) ||
                                           (AnimState->Type == AnimState_Animation));
                    
                    b32 StateShouldBeExited = true;
                    StateShouldBeExited &= (IndicesToUpdate.Count > 0);
                    
                    // NOTE(Dima): Clearing transforms in anim state to safely contribute
                    // NOTE(Dima): all in-state animations
                    ClearNodeTransforms(&Slot->ResultedTransforms, Model->NodeCount);
                    
                    Slot->AdvancedP = {};
                    Slot->AdvancedR = {};
                    Slot->AdvancedS = {};
                    
                    for(int AnimIndexIndex = 0; 
                        AnimIndexIndex < IndicesToUpdate.Count;
                        AnimIndexIndex++)
                    {
                        int AnimIndex = IndicesToUpdate.Indices[AnimIndexIndex];
                        playing_anim* Playing = Slot->PlayingAnimations[AnimIndex];
                        
                        Assert(Playing->AnimationID != 0);
                        
                        animation_clip* Animation = LoadAnimationClip(Assets, 
                                                                      Playing->AnimationID,
                                                                      ASSET_IMPORT_IMMEDIATE);
                        Assert(Animation->NodesCheckSum == AC->NodesCheckSum);
                        
                        // NOTE(Dima): Updating animation and node transforms
                        u32 UpdateResult = UpdatePlayingAnimation(Assets, Model, 
                                                                  Playing, Animation, 
                                                                  GlobalTime, PlaybackRate);
                        
                        if(Animation->UsesRootMotion){
                            Slot->AdvancedP = Playing->AdvancedP;
                            Slot->AdvancedR = Playing->AdvancedR;
                            Slot->AdvancedS = Playing->AdvancedS;
                        }
                        
                        b32 ThisAnimShouldExit = false;
                        
                        switch(UpdateResult){
                            case UpdatePA_ExitState:{
                                ThisAnimShouldExit = true;
                            }break;
                            
                            case UpdatePA_StopPlayingAnimation:{
                                
                            }break;
                            
                            case UpdatePA_PickNext:{
                                if(IsQueueOrSingle){
                                    Slot->PlayingAnimIndex =(Slot->PlayingAnimIndex + 1) % Slot->PlayingAnimationsCount;
                                    
                                    PlayAnimationInternal(Slot, Slot->PlayingAnimIndex,
                                                          GlobalTime, 0.0f);
                                    IndicesToUpdate.Indices[0] = Slot->PlayingAnimIndex;
                                }
                            }break;
                            
                            case UpdatePA_PickRandom:{
                                if(IsQueueOrSingle && IndicesToUpdate.Count > 0){
                                    // NOTE(Dima): Swapping currently playing with the last
                                    playing_anim* Temp = Slot->PlayingAnimations[Slot->PlayingAnimationsCount - 1];
                                    Slot->PlayingAnimations[Slot->PlayingAnimationsCount - 1] = 
                                        Slot->PlayingAnimations[Slot->PlayingAnimIndex];
                                    Slot->PlayingAnimations[Slot->PlayingAnimIndex] = Temp;
                                    
                                    anim_system* System = AC->Control->AnimSystem;
                                    
                                    // NOTE(Dima): Now pick any except the last
                                    Slot->PlayingAnimIndex = GetRandomBetween(&System->Random, 0, 
                                                                              Slot->PlayingAnimationsCount - 1);
                                    
                                    PlayAnimationInternal(Slot, 
                                                          Slot->PlayingAnimIndex,
                                                          GlobalTime, 0.0f);
                                    
                                    IndicesToUpdate.Indices[0] = Slot->PlayingAnimIndex;
                                }
                            }break;
                        }
                        
                        StateShouldBeExited &= ThisAnimShouldExit;
                        
                        // NOTE(Dima): Updated resulted graph node transforms
                        node_transforms_block* Dst = &Slot->ResultedTransforms;
                        node_transforms_block* Src = &Playing->NodeTransforms;
                        
#if !defined(JOY_AVX)            
                        for(int NodeIndex = 0; 
                            NodeIndex < Model->NodeCount;
                            NodeIndex++)
                        {
                            Dst->Ts[NodeIndex] += Src->Ts[NodeIndex];
                            Dst->Rs[NodeIndex] += Src->Rs[NodeIndex];
                            Dst->Ss[NodeIndex] += Src->Ss[NodeIndex];
                        }
#else
                        for(int NodeIndex = 0; 
                            NodeIndex < Model->NodeCount;
                            NodeIndex+=8)
                        {
                            v3_8x SrcT = V3_8X(Src->Ts[NodeIndex + 0],
                                               Src->Ts[NodeIndex + 1],
                                               Src->Ts[NodeIndex + 2],
                                               Src->Ts[NodeIndex + 3],
                                               Src->Ts[NodeIndex + 4],
                                               Src->Ts[NodeIndex + 5],
                                               Src->Ts[NodeIndex + 6],
                                               Src->Ts[NodeIndex + 7]);
                            
                            v4_8x SrcR = V4_8X(Src->Rs[NodeIndex + 0],
                                               Src->Rs[NodeIndex + 1],
                                               Src->Rs[NodeIndex + 2],
                                               Src->Rs[NodeIndex + 3],
                                               Src->Rs[NodeIndex + 4],
                                               Src->Rs[NodeIndex + 5],
                                               Src->Rs[NodeIndex + 6],
                                               Src->Rs[NodeIndex + 7]);
                            
                            v3_8x SrcS = V3_8X(Src->Ss[NodeIndex + 0],
                                               Src->Ss[NodeIndex + 1],
                                               Src->Ss[NodeIndex + 2],
                                               Src->Ss[NodeIndex + 3],
                                               Src->Ss[NodeIndex + 4],
                                               Src->Ss[NodeIndex + 5],
                                               Src->Ss[NodeIndex + 6],
                                               Src->Ss[NodeIndex + 7]);
                            
                            v3_8x DstT = V3_8X(Dst->Ts[NodeIndex + 0],
                                               Dst->Ts[NodeIndex + 1],
                                               Dst->Ts[NodeIndex + 2],
                                               Dst->Ts[NodeIndex + 3],
                                               Dst->Ts[NodeIndex + 4],
                                               Dst->Ts[NodeIndex + 5],
                                               Dst->Ts[NodeIndex + 6],
                                               Dst->Ts[NodeIndex + 7]);
                            
                            v4_8x DstR = V4_8X(Dst->Rs[NodeIndex + 0],
                                               Dst->Rs[NodeIndex + 1],
                                               Dst->Rs[NodeIndex + 2],
                                               Dst->Rs[NodeIndex + 3],
                                               Dst->Rs[NodeIndex + 4],
                                               Dst->Rs[NodeIndex + 5],
                                               Dst->Rs[NodeIndex + 6],
                                               Dst->Rs[NodeIndex + 7]);
                            
                            v3_8x DstS = V3_8X(Dst->Ss[NodeIndex + 0],
                                               Dst->Ss[NodeIndex + 1],
                                               Dst->Ss[NodeIndex + 2],
                                               Dst->Ss[NodeIndex + 3],
                                               Dst->Ss[NodeIndex + 4],
                                               Dst->Ss[NodeIndex + 5],
                                               Dst->Ss[NodeIndex + 6],
                                               Dst->Ss[NodeIndex + 7]);
                            
                            DstT += SrcT;
                            DstS += SrcS;
                            DstR += SrcR;
                            
                            V3_8X_Store(DstT, 
                                        &Dst->Ts[NodeIndex + 0],
                                        &Dst->Ts[NodeIndex + 1],
                                        &Dst->Ts[NodeIndex + 2],
                                        &Dst->Ts[NodeIndex + 3],
                                        &Dst->Ts[NodeIndex + 4],
                                        &Dst->Ts[NodeIndex + 5],
                                        &Dst->Ts[NodeIndex + 6],
                                        &Dst->Ts[NodeIndex + 7]);
                            V3_8X_Store(DstS, 
                                        &Dst->Ss[NodeIndex + 0],
                                        &Dst->Ss[NodeIndex + 1],
                                        &Dst->Ss[NodeIndex + 2],
                                        &Dst->Ss[NodeIndex + 3],
                                        &Dst->Ss[NodeIndex + 4],
                                        &Dst->Ss[NodeIndex + 5],
                                        &Dst->Ss[NodeIndex + 6],
                                        &Dst->Ss[NodeIndex + 7]);
                            V4_8X_Store(DstR,
                                        &Dst->Rs[NodeIndex + 0],
                                        &Dst->Rs[NodeIndex + 1],
                                        &Dst->Rs[NodeIndex + 2],
                                        &Dst->Rs[NodeIndex + 3],
                                        &Dst->Rs[NodeIndex + 4],
                                        &Dst->Rs[NodeIndex + 5],
                                        &Dst->Rs[NodeIndex + 6],
                                        &Dst->Rs[NodeIndex + 7]);
                        }
#endif
                    }
                    
                    b32 IsAlreadyTransitioning = PlayingStateIndex != AC->PlayingIndex;
                    if(StateShouldBeExited && !IsAlreadyTransitioning){
                        anim_transition_request* TryRequest = &AC->TryTransitionRequest;
                        
                        TryRequest->ToState = 0;
                        if(AnimState->FirstTransition && AnimState->FirstTransition->ToState)
                        {
                            TryRequest->ToState = AnimState->FirstTransition->ToState;
                        }
                        
                        TryRequest->Requested = true;
                        TryRequest->TimeToTransit = 0.15f;
                        TryRequest->Phase = 0.0f;
                    }
                }
                
                PlayingStateIndex = GetPrevPlayingIndex(PlayingStateIndex);
            }
            
            {
                BLOCK_TIMING("UpdateAC::Contribute");
                
                // NOTE(Dima): Clearing transforms in anim controller to safely contribute
                // NOTE(Dima): all in-controller playing states
                ClearNodeTransforms(&AC->ResultedTransforms, Model->NodeCount);
                
                // NOTE(Dima): Sum every state resulted transforms based on contribution factor
                int PlayingStateIndex = AC->PlayingIndex;
                
                AC->AdvancedByRootP = {};
                AC->AdvancedByRootR = {};
                AC->AdvancedByRootS = {};
                
                for(int Index = 0;
                    Index < AC->PlayingStatesCount;
                    Index++)
                {
                    playing_state_slot* Slot = &AC->PlayingStates[PlayingStateIndex];
                    anim_state* AnimState = Slot->State;
                    
                    node_transforms_block* Dst = &AC->ResultedTransforms;
                    node_transforms_block* Src = &Slot->ResultedTransforms;
                    
                    AC->AdvancedByRootP += Slot->AdvancedP * Slot->Contribution;
                    AC->AdvancedByRootS += Slot->AdvancedS * Slot->Contribution;
                    float RootAdvanceCheckR = SignNotZero(Dot(AC->AdvancedByRootR, Slot->AdvancedR));
                    AC->AdvancedByRootR += Slot->AdvancedR * Slot->Contribution * RootAdvanceCheckR;
                    
#if !defined(JOY_AVX)
                    float Contribution = Slot->Contribution;
                    
                    for(int NodeIndex = 0; 
                        NodeIndex < Model->NodeCount;
                        NodeIndex++)
                    {
                        // NOTE(Dima): Summing translation
                        Dst->Ts[NodeIndex] += Src->Ts[NodeIndex] * Contribution;
                        
                        // NOTE(Dima): Summing scaling
                        Dst->Ss[NodeIndex] += Src->Ss[NodeIndex] * Contribution;
                        
                        // NOTE(Dima): Summing rotation
                        float RotDot = Dot(Dst->Rs[NodeIndex], 
                                           Src->Rs[NodeIndex]);
                        float SignDot = SignNotZero(RotDot);
                        Dst->Rs[NodeIndex] += Src->Rs[NodeIndex] * SignDot * Contribution;
                    }
#else
                    f32_8x Contribution = F32_8X(Slot->Contribution);
                    
                    for(int NodeIndex = 0; 
                        NodeIndex < Model->NodeCount;
                        NodeIndex+=8)
                    {
                        v3_8x SrcT = V3_8X(Src->Ts[NodeIndex + 0],
                                           Src->Ts[NodeIndex + 1],
                                           Src->Ts[NodeIndex + 2],
                                           Src->Ts[NodeIndex + 3],
                                           Src->Ts[NodeIndex + 4],
                                           Src->Ts[NodeIndex + 5],
                                           Src->Ts[NodeIndex + 6],
                                           Src->Ts[NodeIndex + 7]);
                        
                        v4_8x SrcR = V4_8X(Src->Rs[NodeIndex + 0],
                                           Src->Rs[NodeIndex + 1],
                                           Src->Rs[NodeIndex + 2],
                                           Src->Rs[NodeIndex + 3],
                                           Src->Rs[NodeIndex + 4],
                                           Src->Rs[NodeIndex + 5],
                                           Src->Rs[NodeIndex + 6],
                                           Src->Rs[NodeIndex + 7]);
                        
                        v3_8x SrcS = V3_8X(Src->Ss[NodeIndex + 0],
                                           Src->Ss[NodeIndex + 1],
                                           Src->Ss[NodeIndex + 2],
                                           Src->Ss[NodeIndex + 3],
                                           Src->Ss[NodeIndex + 4],
                                           Src->Ss[NodeIndex + 5],
                                           Src->Ss[NodeIndex + 6],
                                           Src->Ss[NodeIndex + 7]);
                        
                        v3_8x DstT = V3_8X(Dst->Ts[NodeIndex + 0],
                                           Dst->Ts[NodeIndex + 1],
                                           Dst->Ts[NodeIndex + 2],
                                           Dst->Ts[NodeIndex + 3],
                                           Dst->Ts[NodeIndex + 4],
                                           Dst->Ts[NodeIndex + 5],
                                           Dst->Ts[NodeIndex + 6],
                                           Dst->Ts[NodeIndex + 7]);
                        
                        v4_8x DstR = V4_8X(Dst->Rs[NodeIndex + 0],
                                           Dst->Rs[NodeIndex + 1],
                                           Dst->Rs[NodeIndex + 2],
                                           Dst->Rs[NodeIndex + 3],
                                           Dst->Rs[NodeIndex + 4],
                                           Dst->Rs[NodeIndex + 5],
                                           Dst->Rs[NodeIndex + 6],
                                           Dst->Rs[NodeIndex + 7]);
                        
                        v3_8x DstS = V3_8X(Dst->Ss[NodeIndex + 0],
                                           Dst->Ss[NodeIndex + 1],
                                           Dst->Ss[NodeIndex + 2],
                                           Dst->Ss[NodeIndex + 3],
                                           Dst->Ss[NodeIndex + 4],
                                           Dst->Ss[NodeIndex + 5],
                                           Dst->Ss[NodeIndex + 6],
                                           Dst->Ss[NodeIndex + 7]);
                        
                        DstT += SrcT * Contribution;
                        DstS += SrcS * Contribution;
                        
                        f32_8x DotRot = Dot(DstR, SrcR);
                        f32_8x SignDot = SignNotZero(DotRot);
                        DstR += (SrcR * SignDot * Contribution);
                        
                        V3_8X_Store(DstT, 
                                    &Dst->Ts[NodeIndex + 0],
                                    &Dst->Ts[NodeIndex + 1],
                                    &Dst->Ts[NodeIndex + 2],
                                    &Dst->Ts[NodeIndex + 3],
                                    &Dst->Ts[NodeIndex + 4],
                                    &Dst->Ts[NodeIndex + 5],
                                    &Dst->Ts[NodeIndex + 6],
                                    &Dst->Ts[NodeIndex + 7]);
                        V3_8X_Store(DstS, 
                                    &Dst->Ss[NodeIndex + 0],
                                    &Dst->Ss[NodeIndex + 1],
                                    &Dst->Ss[NodeIndex + 2],
                                    &Dst->Ss[NodeIndex + 3],
                                    &Dst->Ss[NodeIndex + 4],
                                    &Dst->Ss[NodeIndex + 5],
                                    &Dst->Ss[NodeIndex + 6],
                                    &Dst->Ss[NodeIndex + 7]);
                        V4_8X_Store(DstR,
                                    &Dst->Rs[NodeIndex + 0],
                                    &Dst->Rs[NodeIndex + 1],
                                    &Dst->Rs[NodeIndex + 2],
                                    &Dst->Rs[NodeIndex + 3],
                                    &Dst->Rs[NodeIndex + 4],
                                    &Dst->Rs[NodeIndex + 5],
                                    &Dst->Rs[NodeIndex + 6],
                                    &Dst->Rs[NodeIndex + 7]);
                    }
#endif
                    PlayingStateIndex = GetPrevPlayingIndex(PlayingStateIndex);
                }
            }
            
            {
                BLOCK_TIMING("UpdateAC::Final");
                
                AC->AdvancedByRootR = Normalize(AC->AdvancedByRootR);
                
                node_transforms_block* Src = &AC->ResultedTransforms;
                
                // NOTE(Dima): Finaling normalization of rotation
#if !defined(JOY_AVX)               
                for(int NodeIndex = 0; 
                    NodeIndex < Model->NodeCount;
                    NodeIndex++)
                {
                    Src->Rs[NodeIndex] = Normalize(Src->Rs[NodeIndex]);
                }
#else
                for(int NodeIndex = 0; 
                    NodeIndex < Model->NodeCount;
                    NodeIndex+=8)
                {
                    v4_8x SrcR = V4_8X(Src->Rs[NodeIndex + 0],
                                       Src->Rs[NodeIndex + 1],
                                       Src->Rs[NodeIndex + 2],
                                       Src->Rs[NodeIndex + 3],
                                       Src->Rs[NodeIndex + 4],
                                       Src->Rs[NodeIndex + 5],
                                       Src->Rs[NodeIndex + 6],
                                       Src->Rs[NodeIndex + 7]);
                    
                    v4_8x Res = Normalize(SrcR);
                    
                    V4_8X_Store(Res,
                                &Src->Rs[NodeIndex + 0],
                                &Src->Rs[NodeIndex + 1],
                                &Src->Rs[NodeIndex + 2],
                                &Src->Rs[NodeIndex + 3],
                                &Src->Rs[NodeIndex + 4],
                                &Src->Rs[NodeIndex + 5],
                                &Src->Rs[NodeIndex + 6],
                                &Src->Rs[NodeIndex + 7]);
                }
#endif
            }
            
            // NOTE(Dima): Calculating to parent transforms
            CalculateToParentTransforms(Model, &AC->ResultedTransforms);
        } // NOTE(Dima): end if transitions count greater than zero
    }
}

INTERNAL_FUNCTION anim_calculated_pose UpdateModelBoneTransforms(asset_system* Assets, 
                                                                 model_info* Model, 
                                                                 animated_component* AC)
{
    FUNCTION_TIMING();
    
    anim_calculated_pose Result = {};
    
    int BoneCount = 0;
    m44* CalculatedBoneTransforms = 0;
    skeleton_info* Skeleton = 0;
    
    if(AC && Model){
        // NOTE(Dima): Getting skeleton if we can
        if(Model->HasSkeleton){
            Skeleton = LoadSkeleton(Assets, Model->SkeletonID, ASSET_IMPORT_DEFERRED);
        }
        
        // NOTE(Dima): Updating skeleton
        if(Skeleton){
            BoneCount = Skeleton->BoneCount;
            CalculatedBoneTransforms = AC->BoneTransformMatrices;
            
#if !defined(JOY_AVX)   
            for(int BoneIndex = 0;
                BoneIndex < BoneCount;
                BoneIndex++)
            {
                bone_info* Bone = &Skeleton->Bones[BoneIndex];
                
                node_info* CorrespondingNode = &Model->Nodes[Bone->NodeIndex];
                
                CalculatedBoneTransforms[BoneIndex] = 
                    Bone->InvBindPose * CorrespondingNode->CalculatedToModel;
            }
#else
            
            for(int BoneIndex = 0;
                BoneIndex < BoneCount;
                BoneIndex += 8)
            {
                i32_8x Indices = IndicesStartFrom(BoneIndex, 1);
                f32_8x IndicesFit = LessThan(ConvertToFloat(Indices), ConvertToFloat(I32_8X(BoneCount)));
                
                if(AnyTrue(IndicesFit)){
                    bone_info* Bone = &Skeleton->Bones[BoneIndex];
                    node_info* CorrespondingNode = &Model->Nodes[Bone->NodeIndex];
                    
                    i32_8x StepIndices = Blend(IndicesFit, Indices, I32_8X(BoneCount - 1));
                    
                    bone_info* Bone0 = &Skeleton->Bones[MMI8(StepIndices.e, 0)];
                    bone_info* Bone1 = &Skeleton->Bones[MMI8(StepIndices.e, 1)];
                    bone_info* Bone2 = &Skeleton->Bones[MMI8(StepIndices.e, 2)];
                    bone_info* Bone3 = &Skeleton->Bones[MMI8(StepIndices.e, 3)];
                    bone_info* Bone4 = &Skeleton->Bones[MMI8(StepIndices.e, 4)];
                    bone_info* Bone5 = &Skeleton->Bones[MMI8(StepIndices.e, 5)];
                    bone_info* Bone6 = &Skeleton->Bones[MMI8(StepIndices.e, 6)];
                    bone_info* Bone7 = &Skeleton->Bones[MMI8(StepIndices.e, 7)];
                    
                    m44_8x InvBindPose = M44_8X(Bone0->InvBindPose,
                                                Bone1->InvBindPose,
                                                Bone2->InvBindPose,
                                                Bone3->InvBindPose,
                                                Bone4->InvBindPose,
                                                Bone5->InvBindPose,
                                                Bone6->InvBindPose,
                                                Bone7->InvBindPose);
                    
                    m44_8x NodeCalcToModel = M44_8X(Model->Nodes[Bone0->NodeIndex].CalculatedToModel,
                                                    Model->Nodes[Bone1->NodeIndex].CalculatedToModel,
                                                    Model->Nodes[Bone2->NodeIndex].CalculatedToModel,
                                                    Model->Nodes[Bone3->NodeIndex].CalculatedToModel,
                                                    Model->Nodes[Bone4->NodeIndex].CalculatedToModel,
                                                    Model->Nodes[Bone5->NodeIndex].CalculatedToModel,
                                                    Model->Nodes[Bone6->NodeIndex].CalculatedToModel,
                                                    Model->Nodes[Bone7->NodeIndex].CalculatedToModel);
                    
                    m44_8x Result = InvBindPose * NodeCalcToModel;
                    
                    M44_8X_Store(Result,
                                 &CalculatedBoneTransforms[MMI8(StepIndices.e, 0)],
                                 &CalculatedBoneTransforms[MMI8(StepIndices.e, 1)],
                                 &CalculatedBoneTransforms[MMI8(StepIndices.e, 2)],
                                 &CalculatedBoneTransforms[MMI8(StepIndices.e, 3)],
                                 &CalculatedBoneTransforms[MMI8(StepIndices.e, 4)],
                                 &CalculatedBoneTransforms[MMI8(StepIndices.e, 5)],
                                 &CalculatedBoneTransforms[MMI8(StepIndices.e, 6)],
                                 &CalculatedBoneTransforms[MMI8(StepIndices.e, 7)]);
                }
            }
#endif
            
            Result.BoneTransforms = CalculatedBoneTransforms;
            Result.BoneTransformsCount = BoneCount;
        }
    }
    
    return(Result);
}

anim_calculated_pose UpdateModelAnimation(asset_system* Assets,
                                          model_info* Model,
                                          animated_component* AnimComp,
                                          f64 GlobalTime,
                                          f32 DeltaTime,
                                          f32 PlaybackRate)
{
    FUNCTION_TIMING();
    
    // NOTE(Dima): Resetting ToParent transforms to default
    ResetToParentTransforms(Model);
    
    /*
    NOTE(dima): Here all animation controller is updated. 
    This includes updating transitions based on incoming variables 
    and transition conditions.
    
    Also interpolating between animations implemented by summing 
    weighted state animations.
    
    At the end of this function ToParentTransform calculated for each node
    */
    UpdateAnimatedComponent(Assets, Model, AnimComp, 
                            GlobalTime, DeltaTime,
                            PlaybackRate);
    
    // NOTE(Dima): This function calculates ToModel transform 
    // NOTE(Dima): based on ToParent of each node
    CalculateToModelTransforms(Model);
    
    // NOTE(Dima): Updating skeleton data
    anim_calculated_pose Result = UpdateModelBoneTransforms(Assets, Model, AnimComp);
    
    return(Result);
}

INTERNAL_FUNCTION anim_controller* AllocateAnimController(anim_system* Anim){
    DLIST_ALLOCATE_FUNCTION_BODY(anim_controller, 
                                 Anim->Arena,
                                 Next, Prev,
                                 Anim->ControlFree,
                                 Anim->ControlUse,
                                 16,
                                 Result);
    
    Result->AnimSystem = Anim;
    
    return(Result);
}

INTERNAL_FUNCTION anim_controller* DeallocateAnimController(anim_system* Anim, 
                                                            anim_controller* Control)
{
    DLIST_DEALLOCATE_FUNCTION_BODY(Control, Next, Prev,
                                   Anim->ControlFree);
    
    return(Control);
}

anim_controller* CreateAnimControl(anim_system* Anim)
{
    anim_controller* Result = AllocateAnimController(Anim);
    
    // NOTE(Dima): Initialize beginned transition to 0
    Result->BeginnedTransitionsCount = 0;
    Result->AnimSourcesCount = 0;
    
    // NOTE(Dima): Init anim state table
    for(int Index = 0;
        Index < ANIM_STATE_TABLE_SIZE;
        Index++)
    {
        Result->StateTable[Index] = 0;
    }
    
    // NOTE(Dima): Initializing state list
    Result->FirstState = 0;
    Result->LastState = 0;
    
    Result->BeginnedState = 0;
    
    return(Result);
}

void InitAnimComponent(animated_component* AC, 
                       anim_controller* Control,
                       u32 NodesCheckSum)
{
    AC->Control = Control;
    
    // NOTE(Dima): Initialize variable table
    for(int Index = 0; 
        Index < ANIM_VAR_TABLE_SIZE;
        Index++)
    {
        AC->VariableHashTable[Index] = 0;
    }
    
    // NOTE(Dima): Initializing variable list
    AC->FirstVariable = 0;
    AC->LastVariable = 0;
    
    // NOTE(Dima): Initializing AnimID table
    for(int Index = 0; 
        Index < ANIM_ANIMID_TABLE_SIZE;
        Index++)
    {
        AC->AnimIDHashTable[Index] = 0;
    }
    
    // NOTE(Dima): Initializing AnimID list
    AC->FirstAnimID = 0;
    AC->LastAnimID = 0;
    
    // NOTE(Dima): Initialize skeleton check sum
    AC->NodesCheckSum = NodesCheckSum;
    
    // NOTE(Dima): Init animations states to play
    for(int SlotIndex = 0;
        SlotIndex < ANIM_MAX_PLAYING_STATES;
        SlotIndex++)
    {
        AC->PlayingStates[SlotIndex].State = 0;
        AC->PlayingStates[SlotIndex].AC = AC;
    }
    
    // NOTE(Dima): Init animations based on animations sources in anim controller
    AC->AnimsCount = Control->AnimSourcesCount;
    AC->Anims = PushArray(Control->AnimSystem->Arena,
                          playing_anim,
                          Control->AnimSourcesCount);
    
    
    for(int AnimationIndex = 0;
        AnimationIndex < Control->AnimSourcesCount;
        AnimationIndex++)
    {
        playing_anim* Anim = &AC->Anims[AnimationIndex];
        playing_anim_source* Source = &Control->AnimSources[AnimationIndex];
        
        Anim->Name = Source->Name;
        Anim->ExitAction = Source->ExitAction;
    }
    
#if 0    
    while(SourceAt){
        
        playing_anim* NewAnimation = AllocatePlayingAnim(Control);
        
        NewAnimation->Name = SourceAt->Name;
        NewAnimation->ExitAction = SourceAt->ExitAction;
        
        // NOTE(Dima): Inserting to state animations list
        NewAnimation->NextInList = 0;
        if((AC->FirstAnimation == 0) && (AC->LastAnimation == 0)){
            AC->FirstAnimation = AC->LastAnimation = NewAnimation;
        }
        else{
            AC->LastAnimation->NextInList = NewAnimation;
            AC->LastAnimation = NewAnimation;
        }
        AC->AnimationsCount++;
        
        SourceAt = SourceAt->NextInList;
    }
#endif
    
    AC->PlayingIndex = 0;
    AC->PlayingStates[AC->PlayingIndex].State = Control->FirstState;
    AC->PlayingStatesCount = (Control->FirstState != 0);
    PlayStateAnimations(&AC->PlayingStates[AC->PlayingIndex], 0.0f, 0.0f);
}

/*
NOTE(Dima): This function selects default playing state(first)
and enables it's all animations
*/
void ClearStateAnimations(animated_component* AC){
    anim_controller* Control = AC->Control;
    
    if(Control){
        
    }
}

// NOTE(Dima): !!!!!!!!!!!!!!!!!
// NOTE(Dima): Anim states stuff
// NOTE(Dima): !!!!!!!!!!!!!!!!!
INTERNAL_FUNCTION anim_state* AllocateAnimState(anim_system* Anim){
    DLIST_ALLOCATE_FUNCTION_BODY(anim_state,
                                 Anim->Arena,
                                 NextAlloc, PrevAlloc,
                                 Anim->StateFree,
                                 Anim->StateUse,
                                 64, 
                                 Result);
    
    Result->NextInHash = 0;
    
    return(Result);
}

INTERNAL_FUNCTION anim_state* DeallocateAnimState(anim_system* Anim,
                                                  anim_state* State)
{
    DLIST_DEALLOCATE_FUNCTION_BODY(State, NextAlloc, PrevAlloc,
                                   Anim->StateFree);
    
    return(State);
}

anim_state* 
FindState(anim_controller* Control, char* Name)
{
    u32 Hash = StringHashFNV(Name);
    
    int EntryIndex = Hash % ANIM_STATE_TABLE_SIZE;
    
    anim_state* Result = 0;
    anim_state* At = Control->StateTable[EntryIndex];
    while(At){
        
        if(StringsAreEqual(At->Name, Name)){
            Result = At;
            break;
        }
        
        At = At->NextInHash;
    }
    
    return(Result);
}

INTERNAL_FUNCTION anim_state* AddAnimStateInternal(anim_controller* Control,
                                                   u32 AnimStateType,
                                                   char* Name)
{
    // NOTE(Dima): Initializing new state
    anim_state* New = AllocateAnimState(Control->AnimSystem);
    
    New->Type = AnimStateType;
    CopyStringsSafe(New->Name, ArrayCount(New->Name), Name);
    New->NextInHash = 0;
    
    // NOTE(Dima): Inserting to hash table
    u32 Hash = StringHashFNV(Name);
    
    int EntryIndex = Hash % ANIM_STATE_TABLE_SIZE;
    
    anim_state* At = Control->StateTable[EntryIndex];
    
    if(At){
        while(At->NextInHash != 0){
            b32 CompareRes = StringsAreEqual(Name, At->Name);
            // NOTE(Dima): Do not allow multiple values with same key
            Assert(!CompareRes);
            
            At = At->NextInHash;
        }
        
        At->NextInHash = New;
    }
    else{
        Control->StateTable[EntryIndex] = New;
    }
    
    // NOTE(Dima): Inserting to list
    New->NextInList = 0;
    if((Control->FirstState == 0) &&
       (Control->LastState == 0))
    {
        Control->FirstState = Control->LastState = New;
    }
    else{
        Control->LastState->NextInList = New;
        Control->LastState = New;
    }
    
    // NOTE(Dima): Init transition list
    New->FirstTransition = 0;
    New->LastTransition = 0;
    
    // NOTE(Dima): Init animations list
    New->FirstAnimIndex = -1;
    New->OnePastLastAnim = -1;
    
    return(New);
}

INTERNAL_FUNCTION void ClearBeginnedStateInControl(anim_controller* Control){
    Assert(Control->BeginnedState);
    Control->BeginnedState = 0;
}

INTERNAL_FUNCTION void AddAnimationInternal(anim_controller* Control,
                                            anim_state* State,
                                            char* Name,
                                            u32 AnimExitAction)
{
    playing_anim_source* NewAnimSource = &Control->AnimSources[Control->AnimSourcesCount];
    Assert(NewAnimSource->IndexInArray < ArrayCount(Control->AnimSources));
    
    CopyStringsSafe(NewAnimSource->Name, 
                    sizeof(NewAnimSource->Name),
                    Name);
    
    NewAnimSource->ExitAction = AnimExitAction;
    
    NewAnimSource->IndexInArray = Control->AnimSourcesCount++;
    
    if(State->FirstAnimIndex == -1){
        // NOTE(Dima): If this is the first anim in this state - init first index
        State->FirstAnimIndex = NewAnimSource->IndexInArray;
        State->OnePastLastAnim = NewAnimSource->IndexInArray;
    }
    State->OnePastLastAnim++;
}

void AddAnimState(anim_controller* Control,
                  char* Name,
                  u32 AnimExitAction)
{
    anim_state* State = AddAnimStateInternal(Control, 
                                             AnimState_Animation, 
                                             Name);
    
    AddAnimationInternal(Control, State,
                         Name, AnimExitAction);
}

void AddQueueAnimation(anim_controller* Control,
                       char* Name,
                       u32 AnimExitAction)
{
    anim_state* BeginnedState = Control->BeginnedState;
    
    if(BeginnedState){
        char AnimationNameTemp[ANIM_DEFAULT_NAME_SIZE];
        
        CopyStringsSafe(AnimationNameTemp, 
                        sizeof(AnimationNameTemp), 
                        BeginnedState->Name);
        AppendCharacterSafe(AnimationNameTemp,
                            sizeof(AnimationNameTemp),
                            ANIM_ANIMATION_NAME_SEPARATOR);
        CopyStringsToEndSafe(AnimationNameTemp,
                             sizeof(AnimationNameTemp),
                             Name);
        
        AddAnimationInternal(Control, BeginnedState,
                             AnimationNameTemp, AnimExitAction);
    }
}

void BeginAnimStateQueue(anim_controller* Control,
                         char* Name)
{
    anim_state* State = AddAnimStateInternal(Control,
                                             AnimState_Queue,
                                             Name);
    
    Assert(!Control->BeginnedState);
    Control->BeginnedState = State;
}

void EndAnimStateQueue(anim_controller* Control)
{
    ClearBeginnedStateInControl(Control);
}


// NOTE(Dima): !!!!!!!!!!!!!!!!
// NOTE(Dima): !Variable stuff!
// NOTE(Dima): !!!!!!!!!!!!!!!!

INTERNAL_FUNCTION anim_variable* AllocateVariable(anim_system* Anim)
{
    DLIST_ALLOCATE_FUNCTION_BODY(anim_variable, 
                                 Anim->Arena,
                                 NextAlloc, PrevAlloc,
                                 Anim->VariableFree,
                                 Anim->VariableUse,
                                 16,
                                 Result);
    
    Result->NextInList = 0;
    Result->NextInHash = 0;
    
    return(Result);
}

INTERNAL_FUNCTION anim_variable* DeallocateVariable(anim_system* Anim, 
                                                    anim_variable* Var)
{
    DLIST_DEALLOCATE_FUNCTION_BODY(Var, 
                                   NextAlloc, PrevAlloc,
                                   Anim->VariableFree);
    
    return(Var);
}

void AddVariable(animated_component* AC,
                 char* Name,
                 u32 VarType)
{
    // NOTE(Dima): Init new variable
    anim_variable* New = AllocateVariable(AC->Control->AnimSystem);
    
    New->ValueType = VarType;
    CopyStringsSafe(New->Name, ArrayCount(New->Name), Name);
    
    // NOTE(Dima): Inserting to hash table
    u32 Hash = StringHashFNV(Name);
    
    int EntryIndex = Hash % ANIM_VAR_TABLE_SIZE;
    
    anim_variable* At = AC->VariableHashTable[EntryIndex];
    
    if(At){
        while(At->NextInHash != 0){
            b32 CompareRes = StringsAreEqual(Name, At->Name);
            // NOTE(Dima): Do not allow multiple values with same key
            Assert(!CompareRes);
            
            At = At->NextInHash;
        }
        
        At->NextInHash = New;
    }
    else{
        AC->VariableHashTable[EntryIndex] = New;
    }
    
    // NOTE(Dima): Inserting to list
    New->NextInList = 0;
    if((AC->FirstVariable == 0) &&
       (AC->LastVariable == 0))
    {
        AC->FirstVariable = AC->LastVariable = New;
    }
    else{
        AC->LastVariable->NextInList = New;
        AC->LastVariable = New;
    }
}

anim_variable* 
FindVariable(animated_component* AC, char* Name)
{
    // NOTE(Dima): Inserting to hash table
    u32 Hash = StringHashFNV(Name);
    
    int EntryIndex = Hash % ANIM_VAR_TABLE_SIZE;
    
    anim_variable* Result = 0;
    anim_variable* At = AC->VariableHashTable[EntryIndex];
    while(At){
        
        if(StringsAreEqual(At->Name, Name)){
            Result = At;
            break;
        }
        
        At = At->NextInHash;
    }
    
    return(Result);
}

// NOTE(Dima): !!!!!!!!!!!!!!
// NOTE(Dima): Anim IDs stuff
// NOTE(Dima): !!!!!!!!!!!!!!

INTERNAL_FUNCTION anim_animid* AllocateAnimID(anim_system* Anim)
{
    DLIST_ALLOCATE_FUNCTION_BODY(anim_animid, 
                                 Anim->Arena,
                                 NextAlloc, PrevAlloc,
                                 Anim->AnimIDFree,
                                 Anim->AnimIDUse,
                                 16,
                                 Result);
    
    Result->NextInList = 0;
    Result->NextInHash = 0;
    
    return(Result);
}

INTERNAL_FUNCTION anim_animid* DeallocateAnimID(anim_system* Anim, 
                                                anim_animid* AnimID)
{
    DLIST_DEALLOCATE_FUNCTION_BODY(AnimID, 
                                   NextAlloc, PrevAlloc,
                                   Anim->AnimIDFree);
    
    return(AnimID);
}

anim_animid* ModifyOrAddAnimID(animated_component* AC,
                               char* Name,
                               u32 AnimationID)
{
    // NOTE(Dima): Init new animid
    anim_animid* New = 0;
    
    // NOTE(Dima): Inserting to hash table
    u32 Hash = StringHashFNV(Name);
    
    int EntryIndex = Hash % ANIM_ANIMID_TABLE_SIZE;
    
    anim_animid* At = AC->AnimIDHashTable[EntryIndex];
    while(At != 0){
        b32 CompareRes = StringsAreEqual(Name, At->Name);
        
        if(CompareRes){
            New = At;
            break;
        }
        
        At = At->NextInHash;
    }
    
    if(New == 0){
        // NOTE(Dima): Allocating
        New = AllocateAnimID(AC->Control->AnimSystem);
        
        CopyStringsSafe(New->Name, ArrayCount(New->Name), Name);
        
        // NOTE(Dima): Inserting to hash table
        New->NextInHash = AC->AnimIDHashTable[EntryIndex];
        AC->AnimIDHashTable[EntryIndex] = New;
        
        // NOTE(Dima): Inserting to list
        New->NextInList = 0;
        if((AC->FirstAnimID == 0) &&
           (AC->LastAnimID == 0))
        {
            AC->FirstAnimID = AC->LastAnimID = New;
        }
        else{
            AC->LastAnimID->NextInList = New;
            AC->LastAnimID = New;
        }
    }
    
    // NOTE(Dima): Setting animation ID
    New->AnimID = AnimationID;
    
    return(New);
}

anim_animid* FindAnimID(animated_component* AC, char* Name){
    anim_animid* Result = 0;
    
    // NOTE(Dima): Inserting to hash table
    u32 Hash = StringHashFNV(Name);
    
    int EntryIndex = Hash % ANIM_ANIMID_TABLE_SIZE;
    
    anim_animid* At = AC->AnimIDHashTable[EntryIndex];
    while(At != 0){
        b32 CompareRes = StringsAreEqual(Name, At->Name);
        
        if(CompareRes){
            Result = At;
            break;
        }
        
        At = At->NextInHash;
    }
    
    return(Result);
}

// NOTE(Dima): !!!!!!!!!!!!!!!!!
// NOTE(Dima): Transitions stuff
// NOTE(Dima): !!!!!!!!!!!!!!!!!

INTERNAL_FUNCTION anim_transition* AllocateTransition(anim_system* Anim)
{
    DLIST_ALLOCATE_FUNCTION_BODY(anim_transition,
                                 Anim->Arena,
                                 NextAlloc, PrevAlloc,
                                 Anim->TransitionFree,
                                 Anim->TransitionUse,
                                 64,
                                 Result);
    
    return(Result);
}

INTERNAL_FUNCTION anim_transition* DeallocateTransition(anim_system* Anim,
                                                        anim_transition* Transition)
{
    DLIST_DEALLOCATE_FUNCTION_BODY(Transition,
                                   NextAlloc, PrevAlloc,
                                   Anim->TransitionFree);
    
    return(Transition);
}

INTERNAL_FUNCTION anim_transition* AddTransitionToStates(anim_controller* Control, 
                                                         anim_state* FromState,
                                                         anim_state* ToState,
                                                         b32 AnimationShouldFinish,
                                                         f32 TimeToTransit,
                                                         f32 TransitPhaseStart)
{
    anim_transition* Result = AllocateTransition(Control->AnimSystem);
    
    Result->ConditionsCount = 0;
    Result->AnimControl = Control;
    
    
    Result->FromState = FromState;
    Result->ToState = ToState;
    
    Result->AnimationShouldFinish = AnimationShouldFinish;
    Result->TimeToTransit = TimeToTransit;
    Result->TransitStartPhase = TransitPhaseStart;
    
    // NOTE(Dima): Inserting to the end of From's transitions list
    Result->NextInList = 0;
    if((FromState->FirstTransition == 0) &&
       (FromState->LastTransition == 0))
    {
        FromState->FirstTransition = FromState->LastTransition = Result;
    }
    else{
        FromState->LastTransition->NextInList = Result;
        FromState->LastTransition = Result;
    }
    
    return(Result);
}

INTERNAL_FUNCTION void
AddTransition(anim_controller* Control, 
              char* FromAnim, 
              char* ToAnim,
              b32 AnimationShouldFinish,
              f32 TimeToTransit,
              f32 TransitPhaseStart)
{
    // NOTE(Dima): Finding to
    anim_state* ToState = FindState(Control, ToAnim);
    Assert(ToState);
    
    // NOTE(Dima): Finding from
    b32 IsAnyState = StringsAreEqual(FromAnim, ANIM_ANY_STATE);
    int TransitionsCount = 0;
    
    if(IsAnyState){
        // NOTE(Dima): If this state is any state - then we should begin to add to all of these states
        anim_state* StateAt = Control->FirstState;
        
        while(StateAt){
            if(!StringsAreEqual(ToAnim, StateAt->Name)){
                Assert(TransitionsCount < ANIM_MAX_STATE_COUNT);
                
                anim_transition* Transition = AddTransitionToStates(Control, 
                                                                    StateAt, 
                                                                    ToState,
                                                                    AnimationShouldFinish,
                                                                    TimeToTransit,
                                                                    TransitPhaseStart);
                
                Control->BeginnedTransitions[TransitionsCount++] = Transition;
            }
            
            StateAt = StateAt->NextInList;
        }
    }
    else{
        anim_state* FromState = FindState(Control, FromAnim);
        Assert(FromState);
        
        anim_transition* Transition = AddTransitionToStates(Control, 
                                                            FromState, 
                                                            ToState,
                                                            AnimationShouldFinish,
                                                            TimeToTransit,
                                                            TransitPhaseStart);
        
        Control->BeginnedTransitions[TransitionsCount++] = Transition;
    }
    
    Assert(TransitionsCount);
    
    Control->BeginnedTransitionsCount = TransitionsCount;
}

void BeginTransition(anim_controller* Control,
                     char* FromAnim, 
                     char* ToAnim, 
                     f32 TimeToTransit,
                     b32 AnimationShouldFinish,
                     f32 TransitPhaseStart)
{
    Assert(Control->BeginnedTransitionsCount == 0);
    AddTransition(Control, FromAnim, ToAnim,
                  AnimationShouldFinish,
                  TimeToTransit,
                  TransitPhaseStart);
}

// NOTE(Dima): !!!!!!!!!!!!!!!!
// NOTE(Dima): Conditions stuff
// NOTE(Dima): !!!!!!!!!!!!!!!!

inline void AddCondition(anim_controller* Control, 
                         char* VariableName,
                         u32 VariableType,
                         u32 ConditionType)
{
    for(int BeginnedTranIndex = 0;
        BeginnedTranIndex < Control->BeginnedTransitionsCount;
        BeginnedTranIndex++)
    {
        anim_transition* Transition = Control->BeginnedTransitions[BeginnedTranIndex];
        
        Assert(Transition->ConditionsCount < MAX_TRANSITION_CONDITIONS);
        anim_transition_condition* Result = &Transition->Conditions[Transition->ConditionsCount++];
        
        CopyStringsSafe(Result->Name, sizeof(Result->Name), VariableName);
        Result->VariableValueType = VariableType;
        Result->ConditionType = ConditionType;
        
        Control->BeginnedTransitionsConditions[BeginnedTranIndex] = Result;
    }
}

void EndTransition(anim_controller* Control){
    Assert(Control->BeginnedTransitionsCount);
    
    Control->BeginnedTransitionsCount = 0;
}

void AddConditionFloat(anim_controller* Control,
                       char* VariableName,
                       u32 ConditionType,
                       f32 Value)
{
    Assert(Control->BeginnedTransitionsCount);
    AddCondition(Control,
                 VariableName,
                 AnimVariable_Float,
                 ConditionType);
    
    for(int BeginnedTranIndex = 0;
        BeginnedTranIndex < Control->BeginnedTransitionsCount;
        BeginnedTranIndex++)
    {
        anim_transition_condition* Cond = Control->BeginnedTransitionsConditions[BeginnedTranIndex];
        
        Cond->Value.Float = Value;
    }
}

void AddConditionBool(anim_controller* Control,
                      char* VariableName,
                      u32 ConditionType,
                      b32 Value)
{
    Assert(Control->BeginnedTransitionsCount);
    AddCondition(Control,
                 VariableName,
                 AnimVariable_Bool,
                 ConditionType);
    
    for(int BeginnedTranIndex = 0;
        BeginnedTranIndex < Control->BeginnedTransitionsCount;
        BeginnedTranIndex++)
    {
        anim_transition_condition* Cond = Control->BeginnedTransitionsConditions[BeginnedTranIndex];
        
        Cond->Value.Bool = Value;
    }
}

void SetFloat(animated_component* AC, 
              char* VariableName, 
              float Value)
{
    anim_variable* FoundVariable = FindVariable(AC, VariableName);
    Assert(FoundVariable);
    
    Assert(FoundVariable->ValueType == AnimVariable_Float);
    
    FoundVariable->Value.Float = Value;
}

void SetBool(animated_component* AC, 
             char* VariableName, 
             b32 Value)
{
    anim_variable* FoundVariable = FindVariable(AC, VariableName);
    Assert(FoundVariable);
    
    Assert(FoundVariable->ValueType == AnimVariable_Bool);
    
    FoundVariable->Value.Bool = Value;
}

void SetAnimationID(animated_component* AC,
                    char* AnimName,
                    u32 AnimationID)
{
    char StateNameTemp[ANIM_DEFAULT_NAME_SIZE];
    
    int DstCounter = 0;
    char* AtDst = StateNameTemp;
    char* AtSrc = AnimName;
    
    if(AnimName){
        while(*AtSrc && (DstCounter < ANIM_DEFAULT_NAME_SIZE - 1)){
            if(*AtSrc == ANIM_ANIMATION_NAME_SEPARATOR){
                break;
            }
            
            *AtDst++ = *AtSrc++;
            
            DstCounter++;
        }
        *AtDst = 0;
        
        anim_state* State = FindState(AC->Control, StateNameTemp);
        if(State){
            ModifyOrAddAnimID(AC, AnimName, AnimationID);
        }
    }
}

void InitAnimSystem(anim_system* Anim)
{
    InitRandomGeneration(&Anim->Random, 123);
    
    // NOTE(Dima): Initializing states sentinels
    DLIST_REFLECT_PTRS(Anim->StateUse, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Anim->StateFree, NextAlloc, PrevAlloc);
    
    // NOTE(Dima): Initializing variable sentinels
    DLIST_REFLECT_PTRS(Anim->VariableUse, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Anim->VariableFree, NextAlloc, PrevAlloc);
    
    // NOTE(Dima): Initializing controllers sentinels
    DLIST_REFLECT_PTRS(Anim->ControlUse, Next, Prev);
    DLIST_REFLECT_PTRS(Anim->ControlFree, Next, Prev);
    
    // NOTE(Dima): Initializing transitions sentinels
    DLIST_REFLECT_PTRS(Anim->TransitionUse, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Anim->TransitionFree, NextAlloc, PrevAlloc);
    
    // NOTE(Dima): Initializing animids sentinels
    DLIST_REFLECT_PTRS(Anim->AnimIDUse, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Anim->AnimIDFree, NextAlloc, PrevAlloc);
    
    DEBUGSetMenuDataSource(DebugMenu_Animation, Anim);
}

