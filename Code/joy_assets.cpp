#include "joy_assets.h"
#include "joy_math.h"
#include "joy_asset_util.h"
#include "joy_software_renderer.h"

#include <vector>
#include <string>

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

INTERNAL_FUNCTION void AddBitmapToAtlas(Asset_Atlas* atlas, 
                                        bmp_info* bmp)
{
    In_Atlas_Bitmap Result = {};
    
    int Border = 3;
    
    int ActualWidth = bmp->Width + Border * 2;
    int ActualHeight = bmp->Height + Border * 2;
    
    if(atlas->AtX + ActualWidth > atlas->Dim){
        atlas->AtX = 0;
        atlas->AtY = atlas->AtY + atlas->MaxInRowHeight;
        atlas->MaxInRowHeight = 0;
    }
    
    ASSERT(atlas->AtY + ActualHeight  < atlas->Dim);
    
    float OneOverDim = atlas->OneOverDim;
    
    for (int YIndex = 0; YIndex < bmp->Height; YIndex++) {
        
        for (int XIndex = 0; XIndex < bmp->Width; XIndex++) {
            u32* At = (u32*)bmp->Pixels + YIndex * bmp->Width + XIndex;
            u32* To = (u32*)atlas->Bitmap.Pixels + (atlas->AtY + YIndex + Border) * atlas->Dim + atlas->AtX + XIndex + Border;
            
            *To = *At;
        }
    }
    
    bmp->MinUV = V2(
        (float)(atlas->AtX + Border) * OneOverDim, 
        (float)(atlas->AtY + Border) * OneOverDim);
    
    bmp->MaxUV = V2(
        (float)(atlas->AtX + Border + bmp->Width) * OneOverDim,
        (float)(atlas->AtY + Border + bmp->Height) * OneOverDim);
    
    atlas->AtX += ActualWidth;
    atlas->MaxInRowHeight = Max(atlas->MaxInRowHeight, ActualHeight);
}

INTERNAL_FUNCTION void AddFontToAtlas(Asset_Atlas* atlas, font_info* font){
    for(int i = 0; i < font->GlyphCount; i++){
        glyph_info* glyph = &font->Glyphs[i];
        
        AddBitmapToAtlas(atlas, &glyph->Bitmap);
    }
}

INTERNAL_FUNCTION Asset_Atlas InitAtlas(mem_region* Region, int Dim){
    Asset_Atlas atlas = {};
    
    mi LargeAtlasMemNeeded = Dim * Dim * 4;
    void* LargeAtlasMem = PushSomeMem(Region, LargeAtlasMemNeeded, 16);
    atlas.Bitmap = AllocateBitmapInternal(
        Dim, 
        Dim,
        LargeAtlasMem);
    atlas.Dim = Dim;
    atlas.AtX = 0;
    atlas.AtY = 0;
    atlas.MaxInRowHeight = 0;
    
    atlas.OneOverDim = 1.0f / (float)Dim;
    
    return(atlas);
}

u32 GetFirstInFamily(assets* Assets, u32 Family){
    asset_family* Fam = &Assets->Families[Family];
    
    u32 Result = Fam->AssetID;
    
    return(Result);
}

INTERNAL_FUNCTION inline void FinalAssetLoading(assets* Assets, asset* Asset)
{
    switch(Asset->Type){
        case AssetType_Font:{
            AddFontToAtlas(&Assets->MainLargeAtlas,
                           GET_ASSET_PTR_MEMBER(Asset, font_info));
        }break;
    }
}

inline asset* AllocateAsset(assets* Assets)
{
    asset_block* PrevBlock = &Assets->AssetBlocks[Assets->CurrentBlockIndex];
    
    int TargetInBlockIndex = PrevBlock->InBlockCount++;
    
    asset_block* CurBlock = PrevBlock;
    
    if(TargetInBlockIndex < MAX_ASSETS_IN_ASSET_BLOCK){
        // NOTE(Dima): Nothing to do!
    }
    else{
        TargetInBlockIndex = 0;
        
        ++Assets->CurrentBlockIndex;
        ASSERT(Assets->CurrentBlockIndex < MAX_ASSET_BLOCKS_COUNT);
        
        CurBlock = &Assets->AssetBlocks[Assets->CurrentBlockIndex];
        CurBlock->InBlockCount = 1;
    }
    
    int AssetBlockIndex = Assets->CurrentBlockIndex;
    
    // NOTE(Dima): If block assets are not allocated yet
    if(!CurBlock->BlockAssets){
        // NOTE(Dima): Allocating
        CurBlock->BlockAssets = PushArray(Assets->Region, asset, MAX_ASSETS_IN_ASSET_BLOCK);
    }
    
    u32 ResultAssetID = 
        (TargetInBlockIndex & 0xFFFF) | 
        ((AssetBlockIndex & 0xFFFF) << 16);
    
    asset* Result = &CurBlock->BlockAssets[TargetInBlockIndex];
    
    Result->Type = AssetType_None;
    Result->State = AssetState_Unloaded;
    Result->DataMemoryEntry = 0;
    Result->ID = ResultAssetID;
    
    return(Result);
}

INTERNAL_FUNCTION added_asset AddAsset(assets* Assets){
    added_asset Result = {};
    
    asset* Asset = AllocateAsset(Assets);
    
    Result.Asset = Asset;
    Result.ID = Asset->ID;
    
    return(Result);
}

void InitAssets(assets* Assets){
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // NOTE(Dima): Memory region is already initialized
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    // NOTE(Dima): Large atlas initialization
    Assets->MainLargeAtlas = InitAtlas(Assets->Region, 1024);
    
    // NOTE(Dima): Temp initialization of asset families
    for(int FamIndex = 0;
        FamIndex < GameAsset_Count;
        FamIndex++)
    {
        asset_family* Fam = &Assets->Families[FamIndex];
        
        asset* AllocatedAsset = AllocateAsset(Assets);
        
        Fam->AssetID = AllocatedAsset->ID;
    }
    
#define TEMP_INIT_FAM_ASSET(fam_id, data_type, value) \
    {\
        asset_family* Fam = &Assets->Families[fam_id];\
        asset* Asset = GetAssetByID(Assets, Fam->AssetID);\
        Asset->Type = AssetType_Type_##data_type;\
        Asset->Data_##data_type = value;\
        Asset->Ptr_##data_type = &Asset->Data_##data_type;\
        FinalAssetLoading(Assets, Asset);}
    
#if 0
    BeginAsset();
    AddBitmap();
    AddBitmap();
    AddBitmap();
    EndAsset();
    
    BeginAsset(Assets, GameAsset_SineSound);
    AddSound(Assets, );
    AddSound(Assets, );
    EndAsset(Assets);
    
    BeginAsset(Assets, GameAsset_Sphere);
    AddMesh(Assets, );
    AddMesh(Assets, );
    AddMesh(Assets, );
    EndAsset(Assets);
    
#endif
    
    TEMP_INIT_FAM_ASSET(GameAsset_CheckboxMark, 
                        bmp_info,
                        LoadBMP("../Data/Icons/checkmark64.png"));
    TEMP_INIT_FAM_ASSET(GameAsset_ChamomileIcon, bmp_info, 
                        LoadBMP("../Data/Icons/chamomile.png"));
    TEMP_INIT_FAM_ASSET(GameAsset_SineTest, sound_info, 
                        MakeSineSound256(44100 * 4, 44100));
    TEMP_INIT_FAM_ASSET(GameAsset_SineTest, sound_info, 
                        MakeSineSound(
        std::vector<int>{256, 128, 430}, 
        44100 * 4, 44100));
    TEMP_INIT_FAM_ASSET(GameAsset_Cube, mesh_info, MakeCube());
    TEMP_INIT_FAM_ASSET(GameAsset_Plane, mesh_info, MakePlane());
    TEMP_INIT_FAM_ASSET(GameAsset_Sphere, mesh_info, MakeSphere(20, 12));
    TEMP_INIT_FAM_ASSET(GameAsset_Cylynder, mesh_info,
                        MakeCylynder(2.0f, 0.5f, 16));
    TEMP_INIT_FAM_ASSET(GameAsset_LiberationMono, font_info, 
                        LoadFont("../Data/Fonts/LiberationMono-Regular.ttf", 18.0f, LoadFont_BakeShadow));
    TEMP_INIT_FAM_ASSET(GameAsset_LilitaOne, font_info, 
                        LoadFont("../Data/Fonts/LilitaOne.ttf", 20.0f, LoadFont_BakeShadow));
    TEMP_INIT_FAM_ASSET(GameAsset_Inconsolata, font_info, 
                        LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 18.0f, LoadFont_BakeBlur));
    TEMP_INIT_FAM_ASSET(GameAsset_PFDIN, font_info, 
                        LoadFont("../Data/Fonts/PFDinTextCondPro-Regular.ttf", 18.0f, LoadFont_BakeBlur));
    TEMP_INIT_FAM_ASSET(GameAsset_MollyJackFont, font_info, 
                        LoadFont("../Data/Fonts/MollyJack.otf", 40.0f, LoadFont_BakeBlur));
}