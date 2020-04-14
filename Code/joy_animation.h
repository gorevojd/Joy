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
    
    b32 Calculated;
};

struct playing_anim{
    f64 GlobalStart;
    f32 PlaybackRate;
    
    // NOTE(Dima): Nodes array is potentially higher size than bones in skeleton
    node_transform NodeTransforms[256];
    
    u32 AnimationID;
};

enum anim_graph_node_type{
    AnimGraphNode_Animation,
    AnimGraphNode_BlendTree,
};

struct anim_graph_node{
    char Name[64];
    
    playing_anim Animation;
    
    anim_graph_node* NextInList;
    
    anim_graph_node* NextAlloc;
    anim_graph_node* PrevAlloc;
    
    anim_graph_node* NextInHash;
    
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
    TransitionCondition_LeesEqThan,
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
    
    struct anim_controller* AnimControl;
    
    anim_transition* NextInList;
    
    anim_transition* NextAlloc;
    anim_transition* PrevAlloc;
};

#define ANIM_GRAPHNODE_TABLE_SIZE 64
#define ANIM_VAR_TABLE_SIZE 32
struct anim_controller{
    struct anim_state* AnimState;
    
    anim_controller* Next;
    anim_controller* Prev;
    
    // NOTE(Dima): Skelton hash
    u32 NodesCheckSum;
    
    anim_graph_node* FirstGraphNode;
    anim_graph_node* LastGraphNode;
    
    anim_variable* FirstVariable;
    anim_variable* LastVariable;
    
    anim_graph_node* GraphNodeTable[ANIM_GRAPHNODE_TABLE_SIZE];
    anim_variable* VariableHashTable[ANIM_VAR_TABLE_SIZE];
    
    // NOTE(Dima): Result transforms array that will be passed to shader
    m44 BoneTransformMatrices[128];
    
    playing_anim* PlayingAnimations[2];
    int PlayingAnimationsCount;
};

struct anim_state{
    mem_region* Region;
    
    f64 GlobalTime;
    
    anim_variable VariableUse;
    anim_variable VariableFree;
    
    anim_controller ControlUse;
    anim_controller ControlFree;
    
    anim_graph_node GraphNodeUse;
    anim_graph_node GraphNodeFree;
    
    anim_transition TransitionUse;
    anim_transition TransitionFree;
};

void ResetToParentTransforms(model_info* Model);
void CalculateToModelTransforms(model_info* Model);

void UpdateModelAnimation(assets* Assets,
                          model_info* Model,
                          playing_anim* PlayingAnim,
                          animation_clip* Animation,
                          f64 CurrentTime);

int UpdateModelBoneTransforms(model_info* Model, 
                              skeleton_info* Skeleton,
                              m44* BoneTransformMatrices);

#define CREATE_ANIM_CONTROL_FUNC(name) anim_controller* name(anim_state* Anim, struct assets* Assets, u32 NodesCheckSum)

anim_controller* CreateAnimControl(anim_state* Anim, 
                                   u32 NodesCheckSum);
void AddAnimGraphNode(anim_controller* Control,
                      u32 AnimGraphNodeType,
                      char* Name);

void AddVariable(anim_controller* Control,
                 char* Name,
                 u32 VarType);

anim_transition* AddTransition(anim_controller* Control, 
                               char* FromAnim, char* ToAnim);

void AddConditionFloat(anim_transition* Transition,
                       char* VariableName,
                       u32 ConditionType,
                       f32 Value);

void AddConditionBool(anim_transition* Transition,
                      char* VariableName,
                      u32 ConditionType,
                      b32 Value);

void SetFloat(anim_controller* Control, 
              char* VariableName, 
              float Value);

void SetBool(anim_controller* Control, 
             char* VariableName, 
             b32 Value);

void InitAnimState(anim_state* Anim);

#endif