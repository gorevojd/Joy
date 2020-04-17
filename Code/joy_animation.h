#ifndef JOY_ANIMATION_H
#define JOY_ANIMATION_H

#include "joy_types.h"
#include "joy_math.h"
#include "joy_assets.h"
#include "joy_strings.h"

struct node_transform{
    v3 T;
    quat R;
    v3 S;
};

struct playing_anim{
    f64 GlobalStart;
    f32 Phase01;
    
    // NOTE(Dima): Nodes array is potentially higher size than bones in skeleton
    node_transform NodeTransforms[256];
    b32 TransformsCalculated[256];
    
    u32 AnimationID;
};

enum anim_state_type{
    AnimState_Animation,
    AnimState_BlendTree,
};

struct anim_state{
    char Name[64];
    
    playing_anim PlayingAnimation;
    
    anim_state* NextInList;
    
    anim_state* NextAlloc;
    anim_state* PrevAlloc;
    
    anim_state* NextInHash;
    
    node_transform ResultedTransforms[256];
    f32 Contribution;
    
    // NOTE(Dima): Transitions list
    struct anim_transition* FirstTransition;
    struct anim_transition* LastTransition;
    
    u32 Type;
};

enum anim_variable_type{
    AnimVariable_Float,
    AnimVariable_Bool,
};

union anim_variable_data{
    f32 Float;
    b32 Bool;
    char* String;
};

struct anim_variable{
    char Name[64];
    
    anim_variable_data Value;
    u32 ValueType;
    
    anim_variable* NextInHash;
    anim_variable* NextInList;
    
    anim_variable* NextAlloc;
    anim_variable* PrevAlloc;
};

enum anim_transition_condition_type{
    TransitionCondition_MoreThan,
    TransitionCondition_MoreEqThan,
    TransitionCondition_LessThan,
    TransitionCondition_LessEqThan,
    TransitionCondition_Equal,
};

struct anim_transition_condition{
    anim_variable_data Value;
    
    anim_variable* Variable;
    
    u32 ConditionType;
};

#define MAX_TRANSITION_CONDITIONS 8
struct anim_transition{
    anim_transition_condition Conditions[MAX_TRANSITION_CONDITIONS];
    int ConditionsCount;
    
    anim_state* FromState;
    anim_state* ToState;
    
    f32 TimeToTransit;
    b32 AnimationShouldExit;
    
    struct anim_controller* AnimControl;
    
    anim_transition* NextInList;
    
    anim_transition* NextAlloc;
    anim_transition* PrevAlloc;
};

#define ANIM_STATE_TABLE_SIZE 64
#define ANIM_VAR_TABLE_SIZE 32
struct anim_controller{
    struct anim_system* AnimState;
    
    anim_controller* Next;
    anim_controller* Prev;
    
    // NOTE(Dima): Skeleton hash
    u32 NodesCheckSum;
    
    anim_state* FirstState;
    anim_state* LastState;
    
    anim_variable* FirstVariable;
    anim_variable* LastVariable;
    
    anim_state* StateTable[ANIM_STATE_TABLE_SIZE];
    anim_variable* VariableHashTable[ANIM_VAR_TABLE_SIZE];
    
#define PLAY_STATE_FIRST 0
#define PLAY_STATE_SECOND 1
    anim_state* PlayingStates[2];
    int PlayingStatesCount = 0;
    f32 TimeToTransit;
    f32 TransitionTimeLeft;
    
    // NOTE(Dima): Result transforms array that will be passed to shader
    node_transform ResultedTransforms[256];
    m44 BoneTransformMatrices[128];
    
    anim_transition* BeginnedTransition;
    
    playing_anim* PlayingAnimations[2];
    int PlayingAnimationsCount;
};

struct anim_system{
    mem_region* Region;
    
    f64 GlobalTime;
    
    anim_variable VariableUse;
    anim_variable VariableFree;
    
    anim_controller ControlUse;
    anim_controller ControlFree;
    
    anim_state StateUse;
    anim_state StateFree;
    
    anim_transition TransitionUse;
    anim_transition TransitionFree;
};

void UpdateModelAnimation(assets* Assets,
                          model_info* Model,
                          anim_controller* Control,
                          f64 GlobalTime,
                          f32 DeltaTime,
                          f32 PlaybackRate);

int UpdateModelBoneTransforms(model_info* Model, 
                              skeleton_info* Skeleton,
                              m44* BoneTransformMatrices);

#define CREATE_ANIM_CONTROL_FUNC(name) anim_controller* name(anim_system* Anim, struct assets* Assets, u32 NodesCheckSum)

anim_controller* CreateAnimControl(anim_system* Anim, 
                                   u32 NodesCheckSum);
void FinalizeCreation(anim_controller* Control);

void AddAnimState(anim_controller* Control,
                  u32 StateType,
                  char* Name);

void AddVariable(anim_controller* Control,
                 char* Name,
                 u32 VarType);

void BeginTransition(anim_controller* Control,
                     char* FromAnim, 
                     char* ToAnim,
                     b32 AnimationShouldExit = true,
                     f32 TimeToTransit = 0.15f);

void EndTransition(anim_controller* Control);

void AddConditionFloat(anim_controller* Control,
                       char* VariableName,
                       u32 ConditionType,
                       f32 Value);

void AddConditionBool(anim_controller* Control,
                      char* VariableName,
                      u32 ConditionType,
                      b32 Value);

void PlayStateAnimations(anim_state* State, 
                         f64 GlobalStart, 
                         f32 Phase);

void SetFloat(anim_controller* Control, 
              char* VariableName, 
              float Value);

void SetBool(anim_controller* Control, 
             char* VariableName, 
             b32 Value);

void SetStateAnimation(anim_controller* Control,
                       char* StateName,
                       u32 AnimationID);

void InitAnimSystem(anim_system* Anim);

#endif