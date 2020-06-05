#ifndef JOY_ANIMATION_H
#define JOY_ANIMATION_H

#include "joy_types.h"
#include "joy_math.h"
#include "joy_assets.h"
#include "joy_strings.h"

#define ANIM_ANY_STATE "#Any"
#define ANIM_TRANSFORMS_ARRAY_SIZE 256
#define ANIM_DEFAULT_TRANSITION_TIME 0.15f
#define ANIM_OPTIMIZE 1

struct find_anim_deltas_ctx{
    // NOTE(Dima): Fail if KeysCount was 0
    b32 Success;
    
    int PrevKeyIndex;
    int NextKeyIndex;
    float t;
};

struct node_transform{
    v3 T;
    quat R;
    v3 S;
};

struct node_transforms_block{
    quat Rs[ANIM_TRANSFORMS_ARRAY_SIZE];
    v3 Ts[ANIM_TRANSFORMS_ARRAY_SIZE];
    v3 Ss[ANIM_TRANSFORMS_ARRAY_SIZE];
};

enum playing_anim_exit_action_type{
    AnimExitAction_Looping,
    AnimExitAction_ExitState,
    AnimExitAction_Next,
};

struct playing_anim{
    f64 GlobalStart;
    f32 Phase01;
    
    b32 TransformsCalculated[ANIM_TRANSFORMS_ARRAY_SIZE];
    node_transforms_block NodeTransforms;
    
    u32 AnimationID;
    
    u32 ExitAction;
};

enum anim_state_type{
    AnimState_Animation,
    AnimState_Queue,
    AnimState_BlendTree,
};

struct anim_state{
    char Name[64];
    
    anim_state* NextInList;
    
    anim_state* NextAlloc;
    anim_state* PrevAlloc;
    
    anim_state* NextInHash;
    
    // NOTE(Dima): Transitions list
    struct anim_transition* FirstTransition;
    struct anim_transition* LastTransition;
    
    u32 Type;
};

struct playing_state_slot{
    anim_state* State;
    
    playing_anim PlayingAnimation;
    
    f32 Contribution;
    
    /*
NOTE(dima): Fot n'th element in playing states
this transition data hold info for transitioning 
from n - 1 -> n
*/
    f32 TimeToTransit;
    f32 TransitionTimeLeft;
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

struct anim_animid{
    char Name[64];
    
    u32 AnimID;
    
    anim_animid* NextAlloc;
    anim_animid* PrevAlloc;
    
    anim_animid* NextInHash;
    anim_animid* NextInList;
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
    
    char Name[64];
    u32 VariableValueType;
    
    u32 ConditionType;
};

#define MAX_TRANSITION_CONDITIONS 8
struct anim_transition{
    anim_transition_condition Conditions[MAX_TRANSITION_CONDITIONS];
    int ConditionsCount;
    
    anim_state* FromState;
    anim_state* ToState;
    
    f32 TimeToTransit;
    b32 AnimationShouldFinish;
    f32 TransitStartPhase;
    
    struct anim_controller* AnimControl;
    
    anim_transition* NextInList;
    
    anim_transition* NextAlloc;
    anim_transition* PrevAlloc;
};

struct anim_calculated_pose{
    m44* BoneTransforms;
    int BoneTransformsCount;
};

#define ANIM_STATE_TABLE_SIZE 64
#define ANIM_VAR_TABLE_SIZE 32
#define ANIM_ANIMID_TABLE_SIZE 32
#define ANIM_MAX_STATE_COUNT 256
#define ANIM_MAX_PLAYING_STATES 128

struct anim_transition_request{
    anim_state* ToState;
    b32 Requested;
    f32 TimeToTransit;
    f32 Phase;
};

struct anim_controller{
    struct anim_system* AnimState;
    
    char Name[64];
    
    anim_controller* Next;
    anim_controller* Prev;
    
    anim_state* FirstState;
    anim_state* LastState;
    anim_state* StateTable[ANIM_STATE_TABLE_SIZE];
    
    anim_transition* BeginnedTransitions[ANIM_MAX_STATE_COUNT];
    anim_transition_condition* BeginnedTransitionsConditions[ANIM_MAX_STATE_COUNT];
    int BeginnedTransitionsCount;
};

struct animated_component{
    anim_controller* Control;
    
    // NOTE(Dima): This table is used to retrieve to set variable by Name
    anim_variable* FirstVariable;
    anim_variable* LastVariable;
    anim_variable* VariableHashTable[ANIM_VAR_TABLE_SIZE];
    
    // NOTE(Dima): This table is used to get/set state or blend tree node animation ID by Name 
    anim_animid* FirstAnimID;
    anim_animid* LastAnimID;
    anim_animid* AnimIDHashTable[ANIM_ANIMID_TABLE_SIZE];
    
    // NOTE(Dima): Skeleton hash
    u32 NodesCheckSum;
    
    playing_state_slot PlayingStates[ANIM_MAX_PLAYING_STATES];
    int PlayingIndex;
    int PlayingStatesCount;
    
    // NOTE(Dima): Result transforms array that will be passed to shader
    node_transforms_block ResultedTransforms;
    m44 BoneTransformMatrices[128];
    
    anim_transition_request ForceTransitionRequest;
};

inline int GetModulatedPlayintIndex(int Index){
    if(Index < 0){
        Index += ((Abs(Index) / ANIM_MAX_PLAYING_STATES) + 1) * ANIM_MAX_PLAYING_STATES;
    }
    
    Index = Index % ANIM_MAX_PLAYING_STATES;
    
    Assert((Index >= 0) && (Index < ANIM_MAX_PLAYING_STATES));
    
    return(Index);
}

inline int GetNextPlayingIndex(int CurIndex){
    int Result = GetModulatedPlayintIndex(CurIndex + 1);
    
    return(Result);
}

inline int GetPrevPlayingIndex(int Index){
    int Result = GetModulatedPlayintIndex(Index - 1);
    
    return(Result);
}

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
    
    anim_animid AnimIDUse;
    anim_animid AnimIDFree;
};

anim_calculated_pose UpdateModelAnimation(assets* Assets,
                                          model_info* Model,
                                          animated_component* AnimComp,
                                          f64 GlobalTime,
                                          f32 DeltaTime,
                                          f32 PlaybackRate);

#define CREATE_ANIM_CONTROL_FUNC(name) anim_controller* name(anim_system* Anim, struct assets* Assets, char* Name)

anim_controller* CreateAnimControl(anim_system* Anim, char* Name, u32 NodesCheckSum);
void FinalizeCreation(anim_controller* Control);

b32 StateIsPlaying(animated_component* AC,
                   char* StateName);
f32 GetPlayingStatePhase(animated_component* AC);

void AddAnimState(anim_controller* Control,
                  u32 StateType,
                  char* Name,
                  f32 EarlyTerminatePhase = 1.0f);

void AddVariable(animated_component* AC,
                 char* Name,
                 u32 VarType);

anim_variable* FindVariable(animated_component* AC, char* Name);
anim_state* FindState(anim_controller* Control, char* Name);
anim_animid* FindAnimID(animated_component* AC, char* Name);

void BeginTransition(anim_controller* Control,
                     char* FromAnim, 
                     char* ToAnim,
                     f32 TimeToTransit = ANIM_DEFAULT_TRANSITION_TIME,
                     b32 AnimationShouldFinish = false,
                     f32 TransitStartPhase = 1.0f);

void EndTransition(anim_controller* Control);

void AddConditionFloat(anim_controller* Control,
                       char* VariableName,
                       u32 ConditionType,
                       f32 Value);

void AddConditionBool(anim_controller* Control,
                      char* VariableName,
                      u32 ConditionType,
                      b32 Value);

void SetFloat(animated_component* AC, 
              char* VariableName, 
              float Value);

void SetBool(animated_component* AC, 
             char* VariableName, 
             b32 Value);

void ForceTransitionRequest(animated_component* AC,
                            char* StateName,
                            f32 TimeToTransit,
                            f32 Phase);

void SetStateAnimation(animated_component* AC,
                       char* StateName,
                       u32 AnimationID);

void InitAnimSystem(anim_system* Anim);

#endif