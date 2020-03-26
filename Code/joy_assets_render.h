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
                                  b32 Immediate = JOY_FALSE)
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
                                 b32 Immediate = JOY_FALSE)
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
                                 b32 Immediate = JOY_FALSE)
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
                                 b32 Immediate = JOY_FALSE)
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

inline model_info* PushOrLoadModel(assets* Assets,
                                   render_stack* Stack,
                                   asset_id ModelID,
                                   v3 P, quat R, v3 S,
                                   b32 Immediate = JOY_FALSE)
{
    asset* Asset = GetAssetByID(Assets, ModelID);
    ASSERT(Asset->Type == AssetType_Model);
    
    LoadAsset(Assets, Asset, Immediate);
    
    model_info* Model = 0;
    if(PotentiallyLoadedAsset(Asset, Immediate)){
        Model= GET_ASSET_PTR_MEMBER(Asset, model_info);
        
        for(int MeshIDIndex = 0; 
            MeshIDIndex < Model->MeshCount;
            MeshIDIndex++)
        {
            asset_id MeshID = Model->MeshIDs[MeshIDIndex];
            
            PushOrLoadMesh(Assets, Stack, MeshID, P, R, S);
        }
        
    }
    
    return(Model);
}

#endif