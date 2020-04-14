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
                          playing_anim* PlayingAnim,
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
            f64 AnimTime = (CurrentTime - PlayingAnim->GlobalStart) * PlayingAnim->PlaybackRate;
            f64 CurrentTick = AnimTime * Animation->TicksPerSecond;
            
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

// NOTE(Dima): Calculating initial node to parent transforms
void ResetToParentTransforms(model_info* Model){
    for(int NodeIndex = 0;
        NodeIndex < Model->NodeCount;
        NodeIndex++)
    {
        node_info* Node = &Model->Nodes[NodeIndex];
        
        Node->CalculatedToParent = Node->Shared->ToParent;
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

INTERNAL_FUNCTION anim_controller* AllocateAnimController(anim_state* Anim){
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

INTERNAL_FUNCTION anim_controller* DeallocateAnimController(anim_state* Anim, 
                                                            anim_controller* Control)
{
    DLIST_DEALLOCATE_FUNCTION_BODY(Control, Next, Prev,
                                   Anim->ControlFree);
    
    return(Control);
}

anim_controller* CreateAnimControl(anim_state* Anim, 
                                   u32 NodesCheckSum)
{
    anim_controller* Result = AllocateAnimController(Anim);
    
    // NOTE(Dima): Initialize skeleton check sum
    Result->NodesCheckSum = NodesCheckSum;
    
    // NOTE(Dima): Initialize variable table
    for(int Index = 0; 
        Index < ANIM_VAR_TABLE_SIZE;
        Index++)
    {
        Result->VariableHashTable[Index] = 0;
    }
    
    // NOTE(Dima): Init graphnode table
    for(int Index = 0;
        Index < ANIM_GRAPHNODE_TABLE_SIZE;
        Index++)
    {
        Result->GraphNodeTable[Index] = 0;
    }
    
    // NOTE(Dima): Initializing graph node list
    Result->FirstGraphNode = 0;
    Result->LastGraphNode = 0;
    
    // NOTE(Dima): Initializing variable list
    Result->FirstVariable = 0;
    Result->LastGraphNode = 0;
    
    return(Result);
}


// NOTE(Dima): !!!!!!!!!!!!!!!!!
// NOTE(Dima): Graph nodes stuff
// NOTE(Dima): !!!!!!!!!!!!!!!!!

INTERNAL_FUNCTION anim_graph_node* AllocateAnimGraphNode(anim_state* Anim){
    DLIST_ALLOCATE_FUNCTION_BODY(anim_graph_node,
                                 Anim->Region,
                                 NextAlloc, PrevAlloc,
                                 Anim->GraphNodeFree,
                                 Anim->GraphNodeUse,
                                 64, 
                                 Result);
    
    Result->NextInHash = 0;
    
    return(Result);
}

INTERNAL_FUNCTION anim_graph_node* DeallocateAnimGraphNode(anim_state* Anim,
                                                           anim_graph_node* GraphNode)
{
    DLIST_DEALLOCATE_FUNCTION_BODY(GraphNode, NextAlloc, PrevAlloc,
                                   Anim->GraphNodeFree);
    
    return(GraphNode);
}

INTERNAL_FUNCTION anim_graph_node* 
FindGraphNode(anim_controller* AC, char* Name)
{
    u32 Hash = StringHashFNV(Name);
    
    int EntryIndex = Hash % ANIM_GRAPHNODE_TABLE_SIZE;
    
    anim_graph_node* Result = 0;
    anim_graph_node* At = AC->GraphNodeTable[EntryIndex];
    while(At){
        
        if(StringsAreEqual(At->Name, Name)){
            Result = At;
            break;
        }
        
        At = At->NextInHash;
    }
    
    return(Result);
}

void AddAnimGraphNode(anim_controller* Control,
                      u32 AnimGraphNodeType,
                      char* Name)
{
    // NOTE(Dima): Initializing new graph node
    anim_graph_node* New = AllocateAnimGraphNode(Control->AnimState);
    
    New->Type = AnimGraphNodeType;
    CopyStringsSafe(New->Name, ArrayCount(New->Name), Name);
    New->NextInHash = 0;
    
    // NOTE(Dima): Inserting to hash table
    u32 Hash = StringHashFNV(Name);
    
    int EntryIndex = Hash % ANIM_GRAPHNODE_TABLE_SIZE;
    
    anim_graph_node* At = Control->GraphNodeTable[EntryIndex];
    
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
        Control->GraphNodeTable[EntryIndex] = New;
    }
    
    // NOTE(Dima): Inserting to list
    New->NextInList = 0;
    if((Control->FirstGraphNode == 0) &&
       (Control->LastGraphNode == 0))
    {
        Control->FirstGraphNode = Control->LastGraphNode = New;
    }
    else{
        Control->LastGraphNode->NextInList = New;
        Control->LastGraphNode = New;
    }
    
    // NOTE(Dima): Init transition list
    New->FirstTransition = 0;
    New->LastTransition = 0;
}


// NOTE(Dima): !!!!!!!!!!!!!!!!
// NOTE(Dima): !Variable stuff!
// NOTE(Dima): !!!!!!!!!!!!!!!!

INTERNAL_FUNCTION anim_variable* AllocateVariable(anim_state* Anim)
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

INTERNAL_FUNCTION anim_variable* DeallocateVariable(anim_state* Anim, 
                                                    anim_variable* Var)
{
    DLIST_DEALLOCATE_FUNCTION_BODY(Var, 
                                   NextAlloc, PrevAlloc,
                                   Anim->VariableFree);
    
    return(Var);
}

void AddVariable(anim_controller* Control,
                 char* Name,
                 u32 VarType)
{
    // NOTE(Dima): Init new variable
    anim_variable* New = AllocateVariable(Control->AnimState);
    
    New->ValueType = VarType;
    CopyStringsSafe(New->Name, ArrayCount(New->Name), Name);
    
    // NOTE(Dima): Inserting to hash table
    u32 Hash = StringHashFNV(Name);
    
    int EntryIndex = Hash % ANIM_VAR_TABLE_SIZE;
    
    anim_variable* At = Control->VariableHashTable[EntryIndex];
    
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
        Control->VariableHashTable[EntryIndex] = New;
    }
    
    // NOTE(Dima): Inserting to list
    New->NextInList = 0;
    if((Control->FirstVariable == 0) &&
       (Control->LastVariable == 0))
    {
        Control->FirstVariable = Control->LastVariable = New;
    }
    else{
        Control->LastVariable->NextInList = New;
        Control->LastVariable = New;
    }
}

INTERNAL_FUNCTION anim_variable* 
FindVariable(anim_controller* AC, char* Name)
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

// NOTE(Dima): !!!!!!!!!!!!!!!!!
// NOTE(Dima): Transitions stuff
// NOTE(Dima): !!!!!!!!!!!!!!!!!

INTERNAL_FUNCTION anim_transition* AllocateTransition(anim_state* Anim)
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

INTERNAL_FUNCTION anim_transition* DeallocateTransition(anim_state* Anim,
                                                        anim_transition* Transition)
{
    DLIST_DEALLOCATE_FUNCTION_BODY(Transition,
                                   NextAlloc, PrevAlloc,
                                   Anim->TransitionFree);
    
    return(Transition);
}

anim_transition* AddTransition(anim_controller* Control, 
                               char* FromAnim, char* ToAnim)
{
    anim_transition* Result = AllocateTransition(Control->AnimState);
    
    Result->ConditionsCount = 0;
    Result->AnimControl = Control;
    
    // NOTE(Dima): Finding from
    anim_graph_node* FromNode = FindGraphNode(Control, FromAnim);
    Assert(FromNode);
    
    // NOTE(Dima): Finding to
    anim_graph_node* ToNode = FindGraphNode(Control, ToAnim);
    Assert(ToNode);
    
    // NOTE(Dima): Inserting to the end of From's transitions list
    Result->NextInList = 0;
    if((FromNode->FirstTransition == 0) &&
       (FromNode->LastTransition == 0))
    {
        FromNode->FirstTransition = FromNode->LastTransition = Result;
    }
    else{
        FromNode->LastTransition->NextInList = Result;
        FromNode->LastTransition = Result;
    }
    
    return(Result);
}


// NOTE(Dima): !!!!!!!!!!!!!!!!
// NOTE(Dima): Conditions stuff
// NOTE(Dima): !!!!!!!!!!!!!!!!

inline anim_transition_condition* AddCondition(anim_transition* Transition,
                                               char* VariableName,
                                               u32 VariableType,
                                               u32 ConditionType)
{
    Assert(Transition->ConditionsCount < MAX_TRANSITION_CONDITIONS);
    anim_transition_condition* Result = &Transition->Conditions[Transition->ConditionsCount++];
    
    anim_variable* FoundVariable = FindVariable(Transition->AnimControl, VariableName);
    Assert(FoundVariable);
    Result->Variable = FoundVariable;
    
    Assert(FoundVariable->ValueType == VariableType);
    
    Result->ConditionType = ConditionType;
    
    return(Result);
}

void AddConditionFloat(anim_transition* Transition,
                       char* VariableName,
                       u32 ConditionType,
                       f32 Value)
{
    anim_transition_condition* Cond = AddCondition(Transition,
                                                   VariableName,
                                                   AnimVariable_Float,
                                                   ConditionType);
    
    Cond->Value.Float = Value;
}

void AddConditionBool(anim_transition* Transition,
                      char* VariableName,
                      u32 ConditionType,
                      b32 Value)
{
    anim_transition_condition* Cond = AddCondition(Transition,
                                                   VariableName,
                                                   AnimVariable_Bool,
                                                   ConditionType);
    
    Cond->Value.Bool = Value;
}

void SetFloat(anim_controller* Control, 
              char* VariableName, 
              float Value)
{
    anim_variable* FoundVariable = FindVariable(Control, VariableName);
    Assert(FoundVariable);
    
    Assert(FoundVariable->ValueType == AnimVariable_Float);
    
    FoundVariable->Value.Float = Value;
}

void SetBool(anim_controller* Control, 
             char* VariableName, 
             b32 Value)
{
    anim_variable* FoundVariable = FindVariable(Control, VariableName);
    Assert(FoundVariable);
    
    Assert(FoundVariable->ValueType == AnimVariable_Bool);
    
    FoundVariable->Value.Bool = Value;
}

void InitAnimState(anim_state* Anim)
{
    // NOTE(Dima): Initializing graph nodes sentinels
    DLIST_REFLECT_PTRS(Anim->GraphNodeUse, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Anim->GraphNodeFree, NextAlloc, PrevAlloc);
    
    // NOTE(Dima): Initializing variable sentinels
    DLIST_REFLECT_PTRS(Anim->VariableUse, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Anim->VariableFree, NextAlloc, PrevAlloc);
    
    // NOTE(Dima): Initializing controllers sentinels
    DLIST_REFLECT_PTRS(Anim->ControlUse, Next, Prev);
    DLIST_REFLECT_PTRS(Anim->ControlFree, Next, Prev);
    
    // NOTE(Dima): Initializing transitions sentinels
    DLIST_REFLECT_PTRS(Anim->TransitionUse, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Anim->TransitionFree, NextAlloc, PrevAlloc);
}
