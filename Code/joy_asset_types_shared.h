#ifndef JOY_ASSET_TYPES_SHARED_H
#define JOY_ASSET_TYPES_SHARED_H

#include "joy_types.h"
#include "joy_math.h"

struct vertex_info{
    v3 P;
    v2 UV;
    v3 N;
    v3 T;
};

struct vertex_skinned_info{
    v3 P;
    v2 UV;
    v3 N;
    v3 T;
    float Weights[4];
    u32 BoneIDs;
};

struct node_shared_data{
    char Name[256];
    
    m44 ToParent;
    m44 ToWorld;
    
    int ParentIndex;
    int FirstChildIndex;
    int ChildCount;
    
    int NodeMeshIndexFirstInStorage;
    int NodeMeshIndexCountInStorage;
};

struct bone_info{
    char Name[256];
    
    m44 InvBindPose;
    
    int ParentIndex;
    int FirstChildIndex;
    int ChildCount;
};


#endif