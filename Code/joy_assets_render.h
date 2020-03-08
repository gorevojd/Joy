#ifndef JOY_ASSETS_RENDER_H
#define JOY_ASSETS_RENDER_H

#include "joy_math.h"
#include "joy_asset_ids.h"
#include "joy_render.h"

inline void PushBitmap(assets* Assets, 
                       render_stack* Stack,
                       v2 P, v2 Dim,
                       asset_id BmpID,
                       v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    asset* Asset = GetAssetByID(Assets, BmpID);
    
    ASSERT(Asset->Type == AssetType_Bitmap);
    
    bmp_info* Bmp = GET_ASSET_PTR_MEMBER(Asset, bmp_info);
    
    if(Bmp){
        PushBitmap(Stack, Bmp, P, Dim.y, ModColor);
    }
}

inline void PushGlyph(assets* Assets, 
                      render_stack* Stack,
                      v2 P, v2 Dim,
                      asset_id BmpID, 
                      v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    asset* Asset = GetAssetByID(Assets, BmpID);
    
    ASSERT(Asset->Type == AssetType_Bitmap);
    
    bmp_info* Bmp = GET_ASSET_PTR_MEMBER(Asset, bmp_info);
    
    v2 MinUV = Bmp->MinUV;
    v2 MaxUV = Bmp->MaxUV;
    
    if(Bmp){
        PushGlyph(Stack, P, Dim, Bmp, 
                  Bmp->MinUV, Bmp->MaxUV,
                  ModColor);
    }
}

inline void PushMesh(assets* Assets, 
                     render_stack* Stack,
                     asset_id MeshID,
                     v3 P,
                     quat R,
                     v3 S)
{
    asset* Asset = GetAssetByID(Assets, MeshID);
    
    ASSERT(Asset->Type == AssetType_Mesh);
    
    mesh_info* Mesh = GET_ASSET_PTR_MEMBER(Asset, mesh_info);
    
    if(Mesh){
        PushMesh(Stack, Mesh, P, R, S);
    }
}

#endif