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
               f32 CurTickTime)
{
    find_anim_deltas_ctx Result = {};
    
    // NOTE(Dima): Finding frame index before CurTickTime
    int FoundPrevIndex = FindPrevFrameIndexForKeyBinary(Times,
                                                        KeysCount,
                                                        CurTickTime);
    
    int LastFrameIndex = KeysCount - 1;
    
    // NOTE(Dima): If not last key frame
    float PrevKeyTime = Times[FoundPrevIndex];
    
    // NOTE(Dima): If found frame is not last
    int NextKeyIndex = FoundPrevIndex + 1;
    f32 TickDistance = Times[NextKeyIndex] - PrevKeyTime;
    
    // NOTE(Dima): Else
    if(FoundPrevIndex == LastFrameIndex)
    {
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
    Result.PrevKeyIndex = FoundPrevIndex;
    Result.NextKeyIndex = NextKeyIndex;
    Result.t = (CurTickTime - PrevKeyTime) / TickDistance;
    
    return(Result);
}

INTERNAL_FUNCTION inline v3 GetAnimatedVector(animation_clip* Animation,
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
        
        // NOTE(Dima): Lerping vectors
        float t = AnimDeltasCtx.t;
        float OneMinusT = 1.0f - t;
        Result = V3(PrevValue.x * OneMinusT + NextValue.x * AnimDeltasCtx.t,
                    PrevValue.y * OneMinusT + NextValue.y * AnimDeltasCtx.t,
                    PrevValue.z * OneMinusT + NextValue.z * AnimDeltasCtx.t);
    }
    
    return(Result);
}

INTERNAL_FUNCTION inline quat GetAnimatedQuat(animation_clip* Animation,
                                              quat* Values,
                                              float* Times,
                                              int KeysCount,
                                              f32 CurTickTime,
                                              quat DefaultValue)
{
    quat Result = DefaultValue;
    
    // NOTE(Dima): Loading first frame's values
    if(KeysCount){
        find_anim_deltas_ctx AnimDeltasCtx = FindAnimDeltas(Animation,
                                                            Times,
                                                            KeysCount,
                                                            CurTickTime);
        
        quat PrevValue = Values[AnimDeltasCtx.PrevKeyIndex];
        quat NextValue = Values[AnimDeltasCtx.NextKeyIndex];
        
        // NOTE(Dima): Lerping quaternion
        Result = Lerp(PrevValue, NextValue, AnimDeltasCtx.t);
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
    
#if 0    
    for(int NodeIndex = 0; 
        NodeIndex < Count; 
        NodeIndex++)
    {
        Transforms->Ts[NodeIndex] = NullVector;
        Transforms->Ss[NodeIndex] = NullVector;
        Transforms->Rs[NodeIndex] = NullQuat;
    }
#else
    
    v3_4x NullVector_4x = V3_4X(NullVector, NullVector, NullVector, NullVector);
    v4_4x NullQuat_4x = V4_4X(NullQuat, NullQuat, NullQuat, NullQuat);
    
    for(int NodeIndex = 0; 
        NodeIndex < Count; 
        NodeIndex+=4)
    {
        V3_4X_Store(NullVector_4x, 
                    &Transforms->Ts[NodeIndex + 0],
                    &Transforms->Ts[NodeIndex + 1],
                    &Transforms->Ts[NodeIndex + 2],
                    &Transforms->Ts[NodeIndex + 3]);
        
        V3_4X_Store(NullVector_4x, 
                    &Transforms->Ss[NodeIndex + 0],
                    &Transforms->Ss[NodeIndex + 1],
                    &Transforms->Ss[NodeIndex + 2],
                    &Transforms->Ss[NodeIndex + 3]);
        
        V4_4X_Store(NullQuat_4x, 
                    &Transforms->Rs[NodeIndex + 0],
                    &Transforms->Rs[NodeIndex + 1],
                    &Transforms->Rs[NodeIndex + 2],
                    &Transforms->Rs[NodeIndex + 3]);
    }
#endif
    
}

INTERNAL_FUNCTION void UpdatePlayingAnimation(assets* Assets,
                                              model_info* Model,
                                              playing_anim* Playing,
                                              animation_clip* Animation,
                                              f64 CurrentTime,
                                              f32 PlaybackRate)
{
    FUNCTION_TIMING();
    
    // NOTE(Dima): Updating animation
    if(Animation){
        for(int NodeIndex = 0;
            NodeIndex < Model->NodeCount;
            NodeIndex++)
        {
            Playing->TransformsCalculated[NodeIndex] = false;
        }
        
        // NOTE(Dima): Animating
        f64 AnimTime = (CurrentTime - Playing->GlobalStart) * PlaybackRate;
        f64 CurrentTick = AnimTime * Animation->TicksPerSecond;
        
        Playing->Phase01 = 0.0f;
        
        if(Animation->DurationTicks > 0.0f){
            if(Animation->IsLooping){
                CurrentTick = fmod(CurrentTick, Animation->DurationTicks);
            }
            
            Playing->Phase01 = Clamp01(CurrentTick / Animation->DurationTicks);
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
            
            {
                BLOCK_TIMING("UpdatePA::Getting");
                
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
                
                int NodeIndex = NodeAnim->NodeIndex;
                Playing->NodeTransforms.Ts[NodeIndex] = AnimatedP;
                Playing->NodeTransforms.Rs[NodeIndex] = AnimatedR;
                Playing->NodeTransforms.Ss[NodeIndex] = AnimatedS;
                Playing->TransformsCalculated[NodeIndex] = true;
            }
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
        
    }
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
#if 0 
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
        NodeIndex < Model->NodeCount - 3;
        NodeIndex+=4)
    {
        m44_4x Tra = M44_4X(TranslationMatrix(Transforms->Ts[NodeIndex + 0]),
                            TranslationMatrix(Transforms->Ts[NodeIndex + 1]),
                            TranslationMatrix(Transforms->Ts[NodeIndex + 2]),
                            TranslationMatrix(Transforms->Ts[NodeIndex + 3]));
        m44_4x Rot = M44_4X(RotationMatrix(Transforms->Rs[NodeIndex + 0]),
                            RotationMatrix(Transforms->Rs[NodeIndex + 1]),
                            RotationMatrix(Transforms->Rs[NodeIndex + 2]),
                            RotationMatrix(Transforms->Rs[NodeIndex + 3]));
        m44_4x Sca = M44_4X(ScalingMatrix(Transforms->Ss[NodeIndex + 0]),
                            ScalingMatrix(Transforms->Ss[NodeIndex + 1]),
                            ScalingMatrix(Transforms->Ss[NodeIndex + 2]),
                            ScalingMatrix(Transforms->Ss[NodeIndex + 3]));
        
        m44_4x Res = Sca * Rot * Tra;
        
        M44_4X_Store(Res, 
                     &Model->Nodes[NodeIndex + 0].CalculatedToParent,
                     &Model->Nodes[NodeIndex + 1].CalculatedToParent,
                     &Model->Nodes[NodeIndex + 2].CalculatedToParent,
                     &Model->Nodes[NodeIndex + 3].CalculatedToParent);
    }
    
    for(NodeIndex; 
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
#endif
}

INTERNAL_FUNCTION void CalculateToModelTransforms(model_info* Model){
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

void PlayStateAnimations(playing_state_slot* Slot, 
                         f64 GlobalStart, 
                         f32 Phase)
{
    Slot->PlayingAnimation.GlobalStart = GlobalStart;
    Slot->PlayingAnimation.Phase01 = Phase;
}

b32 StateIsPlaying(animated_component* AC,
                   char* StateName)
{
    b32 Result = false;
    
    for(int PlayingSlotIndex = 0;
        PlayingSlotIndex < ANIM_MAX_PLAYING_STATES;
        PlayingSlotIndex++)
    {
        anim_state* State = AC->PlayingStates[PlayingSlotIndex].State;
        if(State && StringsAreEqual(State->Name, StateName)){
            Result = true;
            break;
        }
    }
    
    return(Result);
}

INTERNAL_FUNCTION void ProcessInitTransitioning(animated_component* AC,
                                                b32 FirstAnimEnded,
                                                f64 GlobalTime)
{
    b32 Transitioning = (AC->PlayingStatesCount == 2);
    
    anim_state* State = AC->PlayingStates[AC->PlayingIndex].State;
    
    // NOTE(Dima): Iterating through all transitions
    anim_transition* TransitionAt = State->FirstTransition;
    while(TransitionAt != 0){
        
        // NOTE(Dima): From state of transition should be equal to current state
        Assert(TransitionAt->FromState == State);
        
        b32 AllConditionsTrue = true;
        
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
        
        if((TransitionAt->AnimationShouldFinish && FirstAnimEnded) || 
           (TransitionAt->ConditionsCount && AllConditionsTrue))
        {
            // NOTE(Dima): Initiating transition
            int NextPlayIndex = GetNextPlayingIndex(AC->PlayingIndex);
            AC->PlayingStates[NextPlayIndex].State = TransitionAt->ToState;
            AC->PlayingIndex = NextPlayIndex;
            AC->PlayingStatesCount++;
            
            
            PlayStateAnimations(&AC->PlayingStates[NextPlayIndex], 
                                GlobalTime,
                                0.0f);
            
            AC->PlayingStates[NextPlayIndex].TimeToTransit = TransitionAt->TimeToTransit;
            AC->PlayingStates[NextPlayIndex].TransitionTimeLeft = TransitionAt->TimeToTransit;
            
            break;
        }
        
        // NOTE(Dima): Advancing iterator
        TransitionAt = TransitionAt->NextInList;
    } // end loop through all transitions
}

INTERNAL_FUNCTION void UpdateAnimatedComponent(assets* Assets,
                                               model_info* Model, 
                                               animated_component* AC,
                                               f64 GlobalTime,
                                               f32 DeltaTime,
                                               f32 PlaybackRate)
{
    FUNCTION_TIMING();
    
    if(AC){
        anim_controller* Control = AC->Control;
        
        b32 FirstAnimEnded = AC->PlayingStates[AC->PlayingIndex].PlayingAnimation.Phase01 > 0.999999f;
        
        ProcessInitTransitioning(AC,
                                 FirstAnimEnded,
                                 GlobalTime);
        
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
                anim_state* State = AC->PlayingStates[PlayingSlotIndex].State;
                if(State){
                    anim_animid* AnimID = FindAnimID(AC, State->Name);
                    
                    if(AnimID){
                        AC->PlayingStates[PlayingSlotIndex].PlayingAnimation.AnimationID = AnimID->AnimID;
                    }
                    else{
                        // TODO(Dima): Do something
                    }
                }
            }
            
            // NOTE(Dima): Update animations and blend trees of all playing graph nodes
            {
                BLOCK_TIMING("UpdateAC:PreContribute");
                
                int PlayingStateIndex = AC->PlayingIndex;
                for(int Index  = 0;
                    Index < AC->PlayingStatesCount;
                    Index++)
                {
                    playing_state_slot* Slot = &AC->PlayingStates[PlayingStateIndex];
                    
                    anim_state* AnimState = Slot->State;
                    playing_anim* Playing = &Slot->PlayingAnimation;
                    
                    Assert(Playing->AnimationID != 0);
                    if(Playing->AnimationID != 0){
                        animation_clip* Animation = LoadAnimationClip(Assets, 
                                                                      Playing->AnimationID,
                                                                      ASSET_IMPORT_IMMEDIATE);
                        
                        Assert(Animation->NodesCheckSum == AC->NodesCheckSum);
                        
                        // NOTE(Dima): Clearing transforms in anim state to safely contribute
                        // NOTE(Dima): all in-state animations
                        ClearNodeTransforms(&Slot->ResultedTransforms, Model->NodeCount);
                        
                        // NOTE(Dima): Updating animation and node transforms
                        UpdatePlayingAnimation(Assets, Model, 
                                               Playing, Animation, 
                                               GlobalTime, PlaybackRate);
                        
                        // NOTE(Dima): Updated resulted graph node transforms
                        
                        node_transforms_block* Dst = &Slot->ResultedTransforms;
                        node_transforms_block* Src = &Playing->NodeTransforms;
                        
#if 0                    
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
                            NodeIndex+=4)
                        {
                            v3_4x SrcT = V3_4X(Src->Ts[NodeIndex + 0],
                                               Src->Ts[NodeIndex + 1],
                                               Src->Ts[NodeIndex + 2],
                                               Src->Ts[NodeIndex + 3]);
                            
                            v4_4x SrcR = V4_4X(Src->Rs[NodeIndex + 0],
                                               Src->Rs[NodeIndex + 1],
                                               Src->Rs[NodeIndex + 2],
                                               Src->Rs[NodeIndex + 3]);
                            
                            v3_4x SrcS = V3_4X(Src->Ss[NodeIndex + 0],
                                               Src->Ss[NodeIndex + 1],
                                               Src->Ss[NodeIndex + 2],
                                               Src->Ss[NodeIndex + 3]);
                            
                            v3_4x DstT = V3_4X(Dst->Ts[NodeIndex + 0],
                                               Dst->Ts[NodeIndex + 1],
                                               Dst->Ts[NodeIndex + 2],
                                               Dst->Ts[NodeIndex + 3]);
                            
                            v4_4x DstR = V4_4X(Dst->Rs[NodeIndex + 0],
                                               Dst->Rs[NodeIndex + 1],
                                               Dst->Rs[NodeIndex + 2],
                                               Dst->Rs[NodeIndex + 3]);
                            
                            v3_4x DstS = V3_4X(Dst->Ss[NodeIndex + 0],
                                               Dst->Ss[NodeIndex + 1],
                                               Dst->Ss[NodeIndex + 2],
                                               Dst->Ss[NodeIndex + 3]);
                            
                            DstT += SrcT;
                            DstS += SrcS;
                            DstR += SrcR;
                            
                            V3_4X_Store(DstT, 
                                        &Dst->Ts[NodeIndex + 0],
                                        &Dst->Ts[NodeIndex + 1],
                                        &Dst->Ts[NodeIndex + 2],
                                        &Dst->Ts[NodeIndex + 3]);
                            V3_4X_Store(DstS, 
                                        &Dst->Ss[NodeIndex + 0],
                                        &Dst->Ss[NodeIndex + 1],
                                        &Dst->Ss[NodeIndex + 2],
                                        &Dst->Ss[NodeIndex + 3]);
                            V4_4X_Store(DstR,
                                        &Dst->Rs[NodeIndex + 0],
                                        &Dst->Rs[NodeIndex + 1],
                                        &Dst->Rs[NodeIndex + 2],
                                        &Dst->Rs[NodeIndex + 3]);
                        }
#endif
                    }
                    
                    PlayingStateIndex = GetPrevPlayingIndex(PlayingStateIndex);
                }
            }
            
            {
                BLOCK_TIMING("UpdateAC::Contribute");
                
                // NOTE(Dima): Clearing transforms in anim controller to safely contribute
                // NOTE(Dima): all in-controller playing states
                ClearNodeTransforms(&AC->ResultedTransforms, Model->NodeCount);
                
                // NOTE(Dima): Sum every state resulted transforms based on contribution factor
                // TODO(Dima): Potentially we can SIMD this loop
                int PlayingStateIndex = AC->PlayingIndex;
                for(int Index = 0;
                    Index < AC->PlayingStatesCount;
                    Index++)
                {
                    playing_state_slot* Slot = &AC->PlayingStates[PlayingStateIndex];
                    anim_state* AnimState = Slot->State;
                    
                    node_transforms_block* Dst = &AC->ResultedTransforms;
                    node_transforms_block* Src = &Slot->ResultedTransforms;
                    
#if 0                    
                    float Contribution = AnimState->Contribution;
                    
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
                    f32_4x Contribution = F32_4x(Slot->Contribution);
                    
                    for(int NodeIndex = 0; 
                        NodeIndex < Model->NodeCount;
                        NodeIndex+=4)
                    {
                        v3_4x SrcT = V3_4X(Src->Ts[NodeIndex + 0],
                                           Src->Ts[NodeIndex + 1],
                                           Src->Ts[NodeIndex + 2],
                                           Src->Ts[NodeIndex + 3]);
                        
                        v4_4x SrcR = V4_4X(Src->Rs[NodeIndex + 0],
                                           Src->Rs[NodeIndex + 1],
                                           Src->Rs[NodeIndex + 2],
                                           Src->Rs[NodeIndex + 3]);
                        
                        v3_4x SrcS = V3_4X(Src->Ss[NodeIndex + 0],
                                           Src->Ss[NodeIndex + 1],
                                           Src->Ss[NodeIndex + 2],
                                           Src->Ss[NodeIndex + 3]);
                        
                        v3_4x DstT = V3_4X(Dst->Ts[NodeIndex + 0],
                                           Dst->Ts[NodeIndex + 1],
                                           Dst->Ts[NodeIndex + 2],
                                           Dst->Ts[NodeIndex + 3]);
                        
                        v4_4x DstR = V4_4X(Dst->Rs[NodeIndex + 0],
                                           Dst->Rs[NodeIndex + 1],
                                           Dst->Rs[NodeIndex + 2],
                                           Dst->Rs[NodeIndex + 3]);
                        
                        v3_4x DstS = V3_4X(Dst->Ss[NodeIndex + 0],
                                           Dst->Ss[NodeIndex + 1],
                                           Dst->Ss[NodeIndex + 2],
                                           Dst->Ss[NodeIndex + 3]);
                        
                        DstT += SrcT * Contribution;
                        DstS += SrcS * Contribution;
                        
                        f32_4x DotRot = Dot(DstR, SrcR);
                        f32_4x SignDot = SignNotZero(DotRot);
                        DstR += (SrcR * SignDot * Contribution);
                        
                        V3_4X_Store(DstT, 
                                    &Dst->Ts[NodeIndex + 0],
                                    &Dst->Ts[NodeIndex + 1],
                                    &Dst->Ts[NodeIndex + 2],
                                    &Dst->Ts[NodeIndex + 3]);
                        V3_4X_Store(DstS, 
                                    &Dst->Ss[NodeIndex + 0],
                                    &Dst->Ss[NodeIndex + 1],
                                    &Dst->Ss[NodeIndex + 2],
                                    &Dst->Ss[NodeIndex + 3]);
                        V4_4X_Store(DstR,
                                    &Dst->Rs[NodeIndex + 0],
                                    &Dst->Rs[NodeIndex + 1],
                                    &Dst->Rs[NodeIndex + 2],
                                    &Dst->Rs[NodeIndex + 3]);
                    }
#endif
                    PlayingStateIndex = GetPrevPlayingIndex(PlayingStateIndex);
                }
            }
            
            {
                BLOCK_TIMING("UpdateAC::Final");
                
                node_transforms_block* Src = &AC->ResultedTransforms;
                
#if 0                
                // TODO(Dima): SIMD this loop too
                for(int NodeIndex = 0; 
                    NodeIndex < Model->NodeCount;
                    NodeIndex++)
                {
                    // NOTE(Dima): Finaling normalization of rotation
                    Src->Rs[NodeIndex] = Normalize(Src->Rs[NodeIndex]);
                }
#else
                for(int NodeIndex = 0; 
                    NodeIndex < Model->NodeCount;
                    NodeIndex+=4)
                {
                    v4_4x SrcR = V4_4X(Src->Rs[NodeIndex + 0],
                                       Src->Rs[NodeIndex + 1],
                                       Src->Rs[NodeIndex + 2],
                                       Src->Rs[NodeIndex + 3]);
                    
                    v4_4x Res = Normalize(SrcR);
                    
                    V4_4X_Store(Res,
                                &Src->Rs[NodeIndex + 0],
                                &Src->Rs[NodeIndex + 1],
                                &Src->Rs[NodeIndex + 2],
                                &Src->Rs[NodeIndex + 3]);
                }
#endif
            }
            
            // NOTE(Dima): Calculating to parent transforms
            CalculateToParentTransforms(Model, &AC->ResultedTransforms);
            
        } // NOTE(Dima): end if transitions count greater than zero
    }
}

INTERNAL_FUNCTION anim_calculated_pose UpdateModelBoneTransforms(assets* Assets, 
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
            
            for(int BoneIndex = 0;
                BoneIndex < BoneCount;
                BoneIndex++)
            {
                bone_info* Bone = &Skeleton->Bones[BoneIndex];
                
                node_info* CorrespondingNode = &Model->Nodes[Bone->NodeIndex];
                
                CalculatedBoneTransforms[BoneIndex] = 
                    Bone->InvBindPose * CorrespondingNode->CalculatedToModel;
            }
            
            Result.BoneTransforms = CalculatedBoneTransforms;
            Result.BoneTransformsCount = BoneCount;
        }
    }
    
    return(Result);
}

anim_calculated_pose UpdateModelAnimation(assets* Assets,
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
    UpdateAnimatedComponent(Assets, Model, 
                            AnimComp, 
                            GlobalTime,
                            DeltaTime,
                            PlaybackRate);
    
    // NOTE(Dima): This function calculates ToModel transform 
    // NOTE(Dima): based on ToParent of each node
    CalculateToModelTransforms(Model);
    
    // NOTE(Dima): Updating skeleton data
    anim_calculated_pose Result = UpdateModelBoneTransforms(Assets, 
                                                            Model, 
                                                            AnimComp);
    
    return(Result);
}

INTERNAL_FUNCTION anim_controller* AllocateAnimController(anim_system* Anim){
    DLIST_ALLOCATE_FUNCTION_BODY(anim_controller, 
                                 Anim->Region,
                                 Next, Prev,
                                 Anim->ControlFree,
                                 Anim->ControlUse,
                                 16,
                                 Result);
    
    Result->AnimState = Anim;
    
    return(Result);
}

INTERNAL_FUNCTION anim_controller* DeallocateAnimController(anim_system* Anim, 
                                                            anim_controller* Control)
{
    DLIST_DEALLOCATE_FUNCTION_BODY(Control, Next, Prev,
                                   Anim->ControlFree);
    
    return(Control);
}

anim_controller* CreateAnimControl(anim_system* Anim, 
                                   char* Name)
{
    anim_controller* Result = AllocateAnimController(Anim);
    
    CopyStringsSafe(Result->Name, sizeof(Result->Name), Name);
    
    // NOTE(Dima): Initialize beginned transition to 0
    Result->BeginnedTransitionsCount = 0;
    
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
    
    // NOTE(Dima): Init animations to play
    for(int SlotIndex = 0;
        SlotIndex < ANIM_MAX_PLAYING_STATES;
        SlotIndex++)
    {
        AC->PlayingStates[SlotIndex].State = 0;
    }
    
    
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
                                 Anim->Region,
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

void AddAnimState(anim_controller* Control,
                  u32 AnimStateType,
                  char* Name)
{
    // NOTE(Dima): Initializing new state
    anim_state* New = AllocateAnimState(Control->AnimState);
    
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
}


// NOTE(Dima): !!!!!!!!!!!!!!!!
// NOTE(Dima): !Variable stuff!
// NOTE(Dima): !!!!!!!!!!!!!!!!

INTERNAL_FUNCTION anim_variable* AllocateVariable(anim_system* Anim)
{
    DLIST_ALLOCATE_FUNCTION_BODY(anim_variable, 
                                 Anim->Region,
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
    anim_variable* New = AllocateVariable(AC->Control->AnimState);
    
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
                                 Anim->Region,
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
        New = AllocateAnimID(AC->Control->AnimState);
        
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
                                 Anim->Region,
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
                                                         float TimeToTransit)
{
    anim_transition* Result = AllocateTransition(Control->AnimState);
    
    Result->ConditionsCount = 0;
    Result->AnimControl = Control;
    
    
    Result->FromState = FromState;
    Result->ToState = ToState;
    
    Result->AnimationShouldFinish = AnimationShouldFinish;
    Result->TimeToTransit = TimeToTransit;
    
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
              f32 TimeToTransit)
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
                                                                    TimeToTransit);
                
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
                                                            TimeToTransit);
        
        Control->BeginnedTransitions[TransitionsCount++] = Transition;
    }
    
    Assert(TransitionsCount);
    
    Control->BeginnedTransitionsCount = TransitionsCount;
}

void BeginTransition(anim_controller* Control,
                     char* FromAnim, 
                     char* ToAnim, 
                     f32 TimeToTransit,
                     b32 AnimationShouldFinish)
{
    Assert(Control->BeginnedTransitionsCount == 0);
    AddTransition(Control, FromAnim, ToAnim,
                  AnimationShouldFinish,
                  TimeToTransit);
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

void SetStateAnimation(animated_component* AC,
                       char* StateName,
                       u32 AnimationID)
{
    anim_state* State = FindState(AC->Control, StateName);
    if(State){
        ModifyOrAddAnimID(AC, StateName, AnimationID);
    }
}

void InitAnimSystem(anim_system* Anim)
{
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
