#ifndef JOY_ASSET_TYPES_SHARED_H
#define JOY_ASSET_TYPES_SHARED_H

#include "joy_types.h"
#include "joy_math.h"
#include "joy_render_primitives.h"

#define MAX_CHANNELS_PER_BONE 4
#define MAX_WEIGHTS_PER_VERTEX 4

#if 0
enum material_texture_type{
    MaterialTexture_Diffuse,
    MaterialTexture_Specular,
    MaterialTexture_Ambient,
    MaterialTexture_Emissive,
    MaterialTexture_Height,
    MaterialTexture_Normals,
    MaterialTexture_Shininess,
    MaterialTexture_Opacity,
    MaterialTexture_Displacement,
    MaterialTexture_Lightmap,
    MaterialTexture_Reflection,
    MaterialTexture_Unknown,
    
    MaterialTexture_Count,
};
#endif

struct vertex_info{
    v3 P;
    v2 UV;
    v3 N;
    v3 T;
};

inline mesh_type_context MeshSimpleType(){
    mesh_type_context Result = {};
    
    Result.MeshType = Mesh_Simple;
    Result.VertexTypeSize = sizeof(vertex_info);
    
    Result.OffsetP = offsetof(vertex_info, P);
    Result.OffsetUV = offsetof(vertex_info, UV);
    Result.OffsetN = offsetof(vertex_info, N);
    Result.OffsetT = offsetof(vertex_info, T);
    
    return(Result);
}

struct vertex_skinned_info{
    v3 P;
    v2 UV;
    v3 N;
    v3 T;
    float Weights[4];
    u32 BoneIDs;
};

inline mesh_type_context MeshSkinnedType(){
    mesh_type_context Result = {};
    
    Result.MeshType = Mesh_Skinned;
    Result.VertexTypeSize = sizeof(vertex_skinned_info);
    
    Result.OffsetP = offsetof(vertex_skinned_info, P);
    Result.OffsetUV = offsetof(vertex_skinned_info, UV);
    Result.OffsetN = offsetof(vertex_skinned_info, N);
    Result.OffsetT = offsetof(vertex_skinned_info, T);
    Result.OffsetWeights = offsetof(vertex_skinned_info, Weights);
    Result.OffsetBoneIDs = offsetof(vertex_skinned_info, BoneIDs);
    
    return(Result);
}

struct animation_vector_key{
    v3 Value;
    float Time;
};

struct animation_quaternion_key{
    quat Value;
    float Time;
};

struct node_shared_data{
    char Name[256];
    
    m44 ToParent;
    
    int ParentIndex;
    int FirstChildIndex;
    int ChildCount;
    
    int NodeMeshIndexFirstInStorage;
    int NodeMeshIndexCountInStorage;
};

struct bone_info{
    m44 InvBindPose;
    
    // NOTE(Dima): I will always can get bone name through NodeIndex in model nodes array
    int NodeIndex;
    
    // NOTE(Dima): Parent bone index
    int ParentIndex;
};

#endif