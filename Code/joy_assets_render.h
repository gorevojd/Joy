#ifndef JOY_ASSETS_RENDER_H
#define JOY_ASSETS_RENDER_H

inline bmp_info* PushOrLoadBitmap(asset_system* Assets, 
                                  render_state* State,
                                  v2 P, v2 Dim,
                                  asset_id BmpID,
                                  v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f),
                                  b32 Immediate = false)
{
    bmp_info* Bmp = LoadBmp(Assets, BmpID, Immediate);
    
    if(Bmp){
        PushBitmap(State, Bmp, P, Dim.y, ModColor);
    }
    
    return(Bmp);
}

inline bmp_info* PushOrLoadGlyph(asset_system* Assets, 
                                 render_state* State,
                                 v2 P, v2 Dim,
                                 asset_id BmpID, 
                                 v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f),
                                 b32 Immediate = false)
{
    bmp_info* Bmp = LoadBmp(Assets, BmpID, Immediate);
    
    if(Bmp){
        v2 MinUV = Bmp->MinUV;
        v2 MaxUV = Bmp->MaxUV;
        
        PushGlyph(State, P, Dim, Bmp, 
                  Bmp->MinUV, Bmp->MaxUV,
                  ModColor);
    }
    
    return(Bmp);
}

inline mesh_info* PushOrLoadMesh(asset_system* Assets, 
                                 render_state* State,
                                 asset_id MeshID,
                                 v3 P, quat R, v3 S,
                                 b32 Immediate = false)
{
    mesh_info* Mesh = LoadMesh(Assets, MeshID, Immediate);
    
    if(Mesh){
        PushMesh(State, Mesh, P, R, S);
    }
    
    return(Mesh);
}

inline mesh_info* PushOrLoadMesh(asset_system* Assets, 
                                 render_state* State,
                                 asset_id MeshID,
                                 m44 Transformation,
                                 b32 Immediate = false)
{
    mesh_info* Mesh = LoadMesh(Assets, MeshID, Immediate);
    
    if(Mesh){
        PushMesh(State, Mesh, Transformation);
    }
    
    return(Mesh);
}

inline model_info* PushModel(asset_system* Assets,
                             render_state* State,
                             model_info* Model,
                             v3 P, quat R, v3 S)
{
    m44 ModelToWorld = ScalingMatrix(S) * RotationMatrix(R) * TranslationMatrix(P);
    
    if(Model->HasSkeleton)
    {
        skeleton_info* Skeleton = LOAD_ASSET(skeleton_info, 
                                             AssetType_Skeleton,
                                             Assets, Model->SkeletonID,
                                             ASSET_IMPORT_DEFERRED);
        
        if(Skeleton){
            asset_id CubeMeshID = GetFirst(Assets, AssetEntry_Cube);
            
            for(int BoneIndex = 0;
                BoneIndex < Skeleton->BoneCount;
                BoneIndex++)
            {
                bone_info* Bone = &Skeleton->Bones[BoneIndex];
                
                v4 BoneP = 
                    V4(0.0f, 0.0f, 0.0f, 1.0f) * 
                    InverseTransformMatrix(Bone->InvBindPose) * 
                    ModelToWorld;
                
                PushOrLoadMesh(Assets, State, 
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
            
            PushOrLoadMesh(Assets, State, MeshID, NodeTran);
        }
        
    }
#endif
    
    return(Model);
}

#endif