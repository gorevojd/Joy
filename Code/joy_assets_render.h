#ifndef JOY_ASSETS_RENDER_H
#define JOY_ASSETS_RENDER_H

#include "joy_math.h"
#include "joy_asset_ids.h"
#include "joy_render.h"
#include "joy_assets.h"

inline b32 AssetIsLoaded(asset* Asset){
    b32 Result = Asset->State == AssetState_Loaded;
    
    return(Result);
}


inline b32 PotentiallyLoadedAsset(asset* Asset, b32 Immediate){
    b32 Result = AssetIsLoaded(Asset) || Immediate;
    
    return(Result);
}

inline bmp_info* PushOrLoadBitmap(assets* Assets, 
                                  render_stack* Stack,
                                  v2 P, v2 Dim,
                                  asset_id BmpID,
                                  v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f),
                                  b32 Immediate = false)
{
    asset* Asset = GetAssetByID(Assets, BmpID);
    ASSERT(Asset->Type == AssetType_Bitmap);
    
    LoadAsset(Assets, Asset, Immediate);
    
    bmp_info* Bmp = 0; 
    if(PotentiallyLoadedAsset(Asset, Immediate)){
        Bmp = GET_ASSET_PTR_MEMBER(Asset, bmp_info);
        
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
    asset* Asset = GetAssetByID(Assets, BmpID);
    ASSERT(Asset->Type == AssetType_Bitmap);
    
    LoadAsset(Assets, Asset, Immediate);
    
    bmp_info* Bmp = 0;
    if(PotentiallyLoadedAsset(Asset, Immediate)){
        Bmp = GET_ASSET_PTR_MEMBER(Asset, bmp_info);
        
        v2 MinUV = Bmp->MinUV;
        v2 MaxUV = Bmp->MaxUV;
        
        PushGlyph(Stack, P, Dim, Bmp, 
                  Bmp->MinUV, Bmp->MaxUV,
                  ModColor);
    }
    
    return(Bmp);
}

inline font_info* PushOrLoadFont(assets* Assets,
                                 asset_id FontID,
                                 b32 Immediate = false)
{
    asset* Asset = GetAssetByID(Assets, FontID);
    ASSERT(Asset->Type == AssetType_Font);
    
    LoadAsset(Assets, Asset, Immediate);
    
    font_info* Font = 0;
    if(PotentiallyLoadedAsset(Asset, Immediate)){
        Font = GET_ASSET_PTR_MEMBER(Asset, font_info);
    }
    
    return(Font);
}


inline mesh_info* PushOrLoadMesh(assets* Assets, 
                                 render_stack* Stack,
                                 asset_id MeshID,
                                 v3 P, quat R, v3 S,
                                 b32 Immediate = false)
{
    asset* Asset = GetAssetByID(Assets, MeshID);
    ASSERT(Asset->Type == AssetType_Mesh);
    
    LoadAsset(Assets, Asset, Immediate);
    
    mesh_info* Mesh = 0;
    if(PotentiallyLoadedAsset(Asset, Immediate)){
        Mesh = GET_ASSET_PTR_MEMBER(Asset, mesh_info);
        
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
    asset* Asset = GetAssetByID(Assets, MeshID);
    ASSERT(Asset->Type == AssetType_Mesh);
    
    LoadAsset(Assets, Asset, Immediate);
    
    mesh_info* Mesh = 0;
    if(PotentiallyLoadedAsset(Asset, Immediate)){
        Mesh = GET_ASSET_PTR_MEMBER(Asset, mesh_info);
        
        
        PushMesh(Stack, Mesh, Transformation);
    }
    
    return(Mesh);
}

inline skeleton_info* PushOrLoadSkeleton(
assets* Assets,
asset_id SkeletonID,
b32 Immediate = false)
{
    asset* Asset = GetAssetByID(Assets, SkeletonID);
    ASSERT(Asset->Type == AssetType_Skeleton);
    
    LoadAsset(Assets, Asset, Immediate);
    
    skeleton_info* Skeleton = 0;
    if(PotentiallyLoadedAsset(Asset, Immediate)){
        Skeleton = GET_ASSET_PTR_MEMBER(Asset, skeleton_info);
    }
    
    return(Skeleton);
}

inline node_info* PushOrLoadNode(
assets* Assets,
asset_id NodeID,
b32 Immediate = false)
{
    asset* Asset = GetAssetByID(Assets, NodeID);
    ASSERT(Asset->Type == AssetType_Node);
    
    LoadAsset(Assets, Asset, Immediate);
    
    node_info* Node = 0;
    if(PotentiallyLoadedAsset(Asset, Immediate)){
        Node = GET_ASSET_PTR_MEMBER(Asset, node_info);
    }
    
    return(Node);
}

inline model_info* PushOrLoadModel(assets* Assets,
                                   render_stack* Stack,
                                   asset_id ModelID,
                                   v3 P, quat R, v3 S,
                                   b32 Immediate = false)
{
    asset* Asset = GetAssetByID(Assets, ModelID);
    ASSERT(Asset->Type == AssetType_Model);
    
    LoadAsset(Assets, Asset, Immediate);
    
    model_info* Model = 0;
    if(PotentiallyLoadedAsset(Asset, Immediate)){
        Model= GET_ASSET_PTR_MEMBER(Asset, model_info);
        
        m44 ModelToWorld = ScalingMatrix(S) * RotationMatrix(R) * TranslationMatrix(P);
        
#if 0        
        for(int SkIndex = 0;
            SkIndex < Model->SkeletonCount;
            SkIndex++)
        {
            asset_id SkeletonID = Model->SkeletonIDs[SkIndex];
            
            skeleton_info* Skeleton = PushOrLoadSkeleton(Assets, SkeletonID);
            
            if(Skeleton){
                asset_id CubeMeshID = GetFirst(Assets, GameAsset_Cube);
                
                for(int BoneIndex = 0;
                    BoneIndex < Skeleton->BoneCount;
                    BoneIndex++)
                {
                    bone_info* Bone = &Skeleton->Bones[BoneIndex];
                    
                    v4 BoneP = V4(0.0f, 0.0f, 0.0f, 1.0f) * Bone->InvBindPose * ModelToWorld;
                    PushOrLoadMesh(Assets, Stack, CubeMeshID, BoneP.xyz, QuatI(), V3(0.3f), ASSET_LOAD_DEFERRED);
                }
            }
        }
#endif
        
        for(int NodeIndex = 0;
            NodeIndex < Model->NodeCount;
            NodeIndex++)
        {
            u32 NodeID = Model->NodeIDs[NodeIndex];
            
            node_info* Node = PushOrLoadNode(Assets, NodeID);
            
            if(Node){
                asset_id CubeMeshID = GetFirst(Assets, GameAsset_Cube);
                
                m44 NodeTran = Node->ToWorld * ModelToWorld;
                
                for(int MeshIndex = 0; MeshIndex < Node->MeshCount; MeshIndex++){
                    int MeshArrayIndex = Node->MeshIndices[MeshIndex];
                    
                    asset_id MeshID = Model->MeshIDs[MeshArrayIndex];
                    
                    PushOrLoadMesh(Assets, Stack, MeshID, NodeTran);
                }
                
                m44 Tran = ScalingMatrix(V3(0.3f)) * NodeTran;
                PushOrLoadMesh(Assets, Stack, CubeMeshID, Tran, ASSET_LOAD_DEFERRED);
            }
        }
        
    }
    
    return(Model);
}

#endif