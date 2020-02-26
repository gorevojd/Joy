#ifndef JOY_ASSETS_RENDER_H
#define JOY_ASSETS_RENDER_H

#include "joy_math.h"
#include "joy_asset_ids.h"
#include "joy_render.h"

inline void PushGlyph(assets* Assets, 
                      render_stack* Stack,
                      v2 P, v2 Dim,
                      ASSET_TYPED_ID(bmp_info) BmpID, 
                      v4 ModColor = V4(1.0f, 1.0f, 1.0f, 1.0f))
{
    asset* Asset = GetAssetByID(Assets, BmpID);
    
    ASSERT(Asset->Type == AssetType_Bitmap);
    
    bmp_info* Bmp = GET_ASSET_PTR_MEMBER(Asset, bmp_info);
    
    v2 MinUV = Bmp->MinUV;
    v2 MaxUV = Bmp->MaxUV;
    
    PushGlyph(Stack, P, Dim, Bmp, 
              Bmp->MinUV, Bmp->MaxUV,
              ModColor);
}

inline void PushMesh(assets* Assets, 
                     render_stack* Stack,
                     ASSET_TYPED_ID(mesh_info) MeshID,
                     v3 P,
                     quat R,
                     v3 S)
{
    asset* Asset = GetAssetByID(Assets, MeshID);
    
    ASSERT(Asset->Type == AssetType_Mesh);
    
    mesh_info* Mesh = GET_ASSET_PTR_MEMBER(Asset, mesh_info);
    
    PushMesh(Stack, Mesh, P, R, S);
}

#endif