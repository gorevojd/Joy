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

struct find_anim_deltas_ctx{
    int PrevKeyIndex;
    int NextKeyIndex;
    float t;
};

INTERNAL_FUNCTION inline find_anim_deltas_ctx
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
    
    f32 TickDistance = 0.0f;
    
    // NOTE(Dima): If found frame is not last
    int NextKeyIndex = FoundPrevIndex + 1;
    TickDistance = Times[NextKeyIndex] - PrevKeyTime;
    
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
}

INTERNAL_FUNCTION void ClearNodeTransforms(node_transform* Transforms, int Count){
    for(int NodeIndex = 0; 
        NodeIndex < Count; 
        NodeIndex++)
    {
        node_transform* Tran = &Transforms[NodeIndex];
        
        Tran->T = V3(0.0f, 0.0f, 0.0f);
        Tran->S = V3(0.0f, 0.0f, 0.0f);
        Tran->R = Quat(0.0f, 0.0f, 0.0f, 0.0f);
    }
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
            
            node_transform* NodeTransform = &Playing->NodeTransforms[NodeAnim->NodeIndex];
            NodeTransform->T = AnimatedP;
            NodeTransform->R = AnimatedR;
            NodeTransform->S = AnimatedS;
            
            Playing->TransformsCalculated[NodeAnim->NodeIndex] = true;
        }
        
        // NOTE(Dima): Extract transforms that were not calculated
        for(int NodeIndex = 0;
            NodeIndex < Model->NodeCount;
            NodeIndex++)
        {
            node_info* Node = &Model->Nodes[NodeIndex];
            
            node_transform* NodeTransform = &Playing->NodeTransforms[NodeIndex];
            
            if(Playing->TransformsCalculated[NodeIndex] == false){
                DecomposeTransformsForNode(Node->Shared->ToParent, NodeTransform);
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
INTERNAL_FUNCTION void CalculateToParentTransforms(model_info* Model, node_transform* Transforms){
    FUNCTION_TIMING();
    
    for(int NodeIndex = 0; 
        NodeIndex < Model->NodeCount;
        NodeIndex++)
    {
        node_info* TargetNode = &Model->Nodes[NodeIndex];
        node_transform* NodeTran = &Transforms[NodeIndex];
        
        // NOTE(Dima): Calculating to parent transform
        TargetNode->CalculatedToParent = 
            ScalingMatrix(NodeTran->S) * 
            RotationMatrix(NodeTran->R) * 
            TranslationMatrix(NodeTran->T);
    }
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

void PlayStateAnimations(anim_state* State, 
                         f64 GlobalStart, 
                         f32 Phase)
{
    State->PlayingAnimation.GlobalStart = GlobalStart;
    State->PlayingAnimation.Phase01 = Phase;
}

b32 StateIsPlaying(animated_component* AC,
                   char* StateName)
{
    b32 Result = false;
    
    if(AC->PlayingStatesCount > 1 && AC->PlayingStates[GetNextPlayingIndex(AC)]){
        Result |= (StringsAreEqual(AC->PlayingStates[GetNextPlayingIndex(AC)]->Name, StateName));
    }
    
    if(AC->PlayingStatesCount > 0 && AC->PlayingStates[AC->PlayingIndex]){
        Result |= (StringsAreEqual(AC->PlayingStates[AC->PlayingIndex]->Name, StateName));
    }
    
    return(Result);
}

INTERNAL_FUNCTION void ProcessTransitioning(animated_component* AC, 
                                            int PlayingIndex,
                                            b32 FirstAnimEnded,
                                            f64 GlobalTime)
{
    Assert(PlayingIndex < 2);
    Assert(PlayingIndex >= 0);
    
    anim_state* State = AC->PlayingStates[PlayingIndex];
    
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
            int NextPlayIndex = GetNextPlayingIndex(AC);
            AC->PlayingStates[NextPlayIndex] = TransitionAt->ToState;
            AC->PlayingStatesCount = 2;
            
            PlayStateAnimations(AC->PlayingStates[NextPlayIndex], 
                                GlobalTime,
                                0.0f);
            
            AC->TimeToTransit = TransitionAt->TimeToTransit;
            AC->TransitionTimeLeft = TransitionAt->TimeToTransit;
            
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
        
        // NOTE(Dima): Setting states animations based on StateName->AnimID mapping
        anim_animid* AnimIDAt = AC->FirstAnimID;
        while(AnimIDAt != 0){
            anim_state* State = FindState(AC->Control, AnimIDAt->Name);
            
            if(State){
                State->PlayingAnimation.AnimationID = AnimIDAt->AnimID;
            }
            
            AnimIDAt = AnimIDAt->NextInList;
        }
        
        // NOTE(Dima): Updating playing states, conditions, transitions
        if(AC->PlayingStatesCount > 0){
            int CurrPlayIndex = AC->PlayingIndex;
            int NextPlayIndex = GetNextPlayingIndex(AC);
            
            // NOTE(Dima): Updating transition
            if(AC->PlayingStatesCount == 2){
                
                // NOTE(Dima): If transitioning
                AC->TransitionTimeLeft -= DeltaTime * PlaybackRate;
                
                b32 ShouldEndTransition = false;
                
                if(AC->TimeToTransit > 0.0f){
                    if(AC->TransitionTimeLeft < 0.0000001f){
                        AC->TransitionTimeLeft = 0.0f;
                        ShouldEndTransition = true;
                    }
                    
                    f32 Weight0 = Clamp01(AC->TransitionTimeLeft / AC->TimeToTransit);
                    f32 Weight1 = 1.0f - Weight0;
                    
                    AC->PlayingStates[CurrPlayIndex]->Contribution = Weight0;
                    AC->PlayingStates[NextPlayIndex]->Contribution = Weight1;
                }
                else{
                    // NOTE(Dima): 0 here because this animation ended
                    AC->PlayingStates[CurrPlayIndex]->Contribution = 0.0f;
                    
                    // NOTE(Dima): 1 here because this animation is fully turned on
                    AC->PlayingStates[NextPlayIndex]->Contribution = 1.0f;
                    
                    ShouldEndTransition = true;
                }
                
                if(ShouldEndTransition){
                    AC->PlayingStates[CurrPlayIndex] = 0;
                    AC->PlayingIndex = NextPlayIndex;
                    AC->PlayingStatesCount = 1;
                    
                    NextPlayIndex = CurrPlayIndex;
                    CurrPlayIndex = AC->PlayingIndex;
                    
                    AC->TimeToTransit = 0.0f;
                    AC->TransitionTimeLeft = 0.0f;
                }
            }
            else if(AC->PlayingStatesCount == 1){
                AC->PlayingStates[CurrPlayIndex]->Contribution = 1.0f;
            }
            
            b32 FirstAnimEnded = AC->PlayingStates[CurrPlayIndex]->PlayingAnimation.Phase01 > 0.999999f;
            
            /*
            PlayingStatesCount can change before this line so 
            we should recalculate if we are transitioning right now
            */
            b32 Transitioning = (AC->PlayingStatesCount == 2);
            if(!Transitioning){
                ProcessTransitioning(AC,
                                     Transitioning ? NextPlayIndex : CurrPlayIndex,
                                     FirstAnimEnded,
                                     GlobalTime);
            }
            
            // NOTE(Dima): Update animations and blend trees of all playing graph nodes
            for(int PlayingStateIndex = 0;
                PlayingStateIndex < AC->PlayingStatesCount;
                PlayingStateIndex++)
            {
                int ToGetIndex = PlayingStateIndex ? GetNextPlayingIndex(AC) : AC->PlayingIndex;
                anim_state* AnimNode = AC->PlayingStates[ToGetIndex];
                
                playing_anim* Playing = &AnimNode->PlayingAnimation;
                
                animation_clip* Animation = LoadAnimationClip(Assets, 
                                                              Playing->AnimationID,
                                                              ASSET_IMPORT_IMMEDIATE);
                
                Assert(Animation->NodesCheckSum == AC->NodesCheckSum);
                
                // NOTE(Dima): Clearing transforms in anim state to safely contribute
                // NOTE(Dima): all in-state animations
                ClearNodeTransforms(AnimNode->ResultedTransforms, Model->NodeCount);
                
                // NOTE(Dima): Updating animation and node transforms
                UpdatePlayingAnimation(Assets, Model, 
                                       Playing, Animation, 
                                       GlobalTime, PlaybackRate);
                
                // NOTE(Dima): Updated resulted graph node transforms
                for(int NodeIndex = 0; 
                    NodeIndex < Model->NodeCount;
                    NodeIndex++)
                {
                    node_transform* Dst = &AnimNode->ResultedTransforms[NodeIndex];
                    node_transform* Src = &Playing->NodeTransforms[NodeIndex];
                    
                    
                    Dst->T += Src->T;
                    Dst->R += Src->R;
                    Dst->S += Src->S;
                }
            }
            
            {
                BLOCK_TIMING("UpdateAC::Contribute");
                
                // NOTE(Dima): Clearing transforms in anim controller to safely contribute
                // NOTE(Dima): all in-controller playing states
                ClearNodeTransforms(AC->ResultedTransforms, Model->NodeCount);
                
                // NOTE(Dima): Sum every state resulted transforms based on contribution factor
                // TODO(Dima): Potentially we can SIMD this loop
                for(int PlayingStateIndex = 0;
                    PlayingStateIndex < AC->PlayingStatesCount;
                    PlayingStateIndex++)
                {
                    int ToGetIndex = PlayingStateIndex ? GetNextPlayingIndex(AC) : AC->PlayingIndex;
                    anim_state* AnimState = AC->PlayingStates[ToGetIndex];
                    
                    float Contribution = AnimState->Contribution;
                    
                    for(int NodeIndex = 0; 
                        NodeIndex < Model->NodeCount;
                        NodeIndex++)
                    {
                        node_transform* Dst = &AC->ResultedTransforms[NodeIndex];
                        node_transform* Src = &AnimState->ResultedTransforms[NodeIndex];
                        
                        // NOTE(Dima): Summing translation
                        Dst->T += Src->T * Contribution;
                        
                        // NOTE(Dima): Summing scaling
                        Dst->S += Src->S * Contribution;
                        
                        // NOTE(Dima): Summing rotation
                        float RotDot = Dot(Dst->R, Src->R);
                        float SignDot = SignNotZero(RotDot);
                        Dst->R += Src->R * SignDot * Contribution;
                    }
                }
            }
            
            {
                BLOCK_TIMING("UpdateAC::Final");
                
                // TODO(Dima): SIMD this loop too
                for(int NodeIndex = 0; 
                    NodeIndex < Model->NodeCount;
                    NodeIndex++)
                {
                    node_info* TargetNode = &Model->Nodes[NodeIndex];
                    node_transform* NodeTran = &AC->ResultedTransforms[NodeIndex];
                    
                    // NOTE(Dima): Finaling normalization of rotation
                    NodeTran->R = Normalize(NodeTran->R);
                }
                
                
                // NOTE(Dima): Calculating to parent transforms
                CalculateToParentTransforms(Model, AC->ResultedTransforms);
            }
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
    AC->PlayingStatesCount = (Control->FirstState != 0);
    AC->PlayingIndex = 0;
    AC->PlayingStates[AC->PlayingIndex] = Control->FirstState;
    AC->PlayingStates[GetNextPlayingIndex(AC)] = 0;
    
    PlayStateAnimations(AC->PlayingStates[AC->PlayingIndex], 0.0f, 0.0f);
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
