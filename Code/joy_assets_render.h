#ifndef JOY_ASSETS_RENDER_H
#define JOY_ASSETS_RENDER_H

#include "joy_math.h"
#include "joy_asset_ids.h"
#include "joy_render.h"
#include "joy_assets.h"

inline bmp_info* PushOrLoadBitmap(assets* Assets, 
                                  render_stack* Stack,
                                  v2 P, v2 Dim,
                                  asset_id BmpID,
                                  v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f),
                                  b32 Immediate = false)
{
    bmp_info* Bmp = LoadBmp(Assets, BmpID, Immediate);
    
    if(Bmp){
        PushBitmap(Stack, Bmp, P, Dim.y, ModColor);
    }
    
    return(Bmp);
}

inline bmp_info* PushOrLoadGlyph(assets* Assets, 
                                 render_stack* Stack,
                                 v2 P, v2 Dim,
                                 asset_id BmpID, 
                                 v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f),
                                 b32 Immediate = false)
{
    bmp_info* Bmp = LoadBmp(Assets, BmpID, Immediate);
    
    if(Bmp){
        v2 MinUV = Bmp->MinUV;
        v2 MaxUV = Bmp->MaxUV;
        
        PushGlyph(Stack, P, Dim, Bmp, 
                  Bmp->MinUV, Bmp->MaxUV,
                  ModColor);
    }
    
    return(Bmp);
}

inline mesh_info* PushOrLoadMesh(assets* Assets, 
                                 render_stack* Stack,
                                 asset_id MeshID,
                                 v3 P, quat R, v3 S,
                                 b32 Immediate = false)
{
    mesh_info* Mesh = LoadMesh(Assets, MeshID, Immediate);
    
    if(Mesh){
        PushMesh(Stack, Mesh, P, R, S);
    }
    
    return(Mesh);
}

inline mesh_info* PushOrLoadMesh(assets* Assets, 
                                 render_stack* Stack,
                                 asset_id MeshID,
                                 m44 Transformation,
                                 b32 Immediate = false)
{
    mesh_info* Mesh = LoadMesh(Assets, MeshID, Immediate);
    
    if(Mesh){
        PushMesh(Stack, Mesh, Transformation);
    }
    
    return(Mesh);
}

inline model_info* PushModel(assets* Assets,
                             render_stack* Stack,
                             model_info* Model,
                             v3 P, quat R, v3 S)
{
    m44 ModelToWorld = ScalingMatrix(S) * RotationMatrix(R) * TranslationMatrix(P);
    
    for(int SkIndex = 0;
        SkIndex < Model->SkeletonCount;
        SkIndex++)
    {
        asset_id SkeletonID = Model->SkeletonIDs[SkIndex];
        
        skeleton_info* Skeleton = LOAD_ASSET(skeleton_info, 
                                             AssetType_Skeleton,
                                             Assets, SkeletonID,
                                             ASSET_IMPORT_DEFERRED);
        
        if(Skeleton){
            asset_id CubeMeshID = GetFirst(Assets, GameAsset_Cube);
            
            for(int BoneIndex = 0;
                BoneIndex < Skeleton->BoneCount;
                BoneIndex++)
            {
                bone_info* Bone = &Skeleton->Bones[BoneIndex];
                
                v4 BoneP = 
                    V4(0.0f, 0.0f, 0.0f, 1.0f) * 
                    InverseTransformMatrix(Bone->InvBindPose) * 
                    ModelToWorld;
                
                PushOrLoadMesh(Assets, Stack, 
                               CubeMeshID, BoneP.xyz, 
                               QuatI(), V3(0.1f), 
                               ASSET_IMPORT_DEFERRED);
            }
        }
    }
    
#if 1        
    for(int NodeIndex = 0;
        NodeIndex < Model->NodeCount;
        NodeIndex++)
    {
        node_info* Node = &Model->Nodes[NodeIndex];
        
        Node->CalculatedToParent = Node->Shared->ToParent;
        if(Node->Shared->ParentIndex != -1){
            // NOTE(Dima): If is not root
            node_info* ParentNode = &Model->Nodes[Node->Shared->ParentIndex];
            
            Node->CalculatedToModel = Node->CalculatedToParent* ParentNode->CalculatedToModel;
        }
        else{
            Node->CalculatedToModel = Node->CalculatedToParent;
        }
        
        m44 NodeTran = Node->CalculatedToModel * ModelToWorld;
        
        for(int MeshIndex = 0; MeshIndex < Node->MeshCount; MeshIndex++){
            asset_id MeshID = Node->MeshIDs[MeshIndex];
            
            PushOrLoadMesh(Assets, Stack, MeshID, NodeTran);
        }
        
    }
#endif
    
    return(Model);
}

#endif