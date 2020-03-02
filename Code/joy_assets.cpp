#include "joy_assets.h"
#include "joy_math.h"
#include "joy_asset_util.h"
#include "joy_software_renderer.h"

#include <vector>
#include <atomic>

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

u32 GetFirst(assets* Assets, u32 Group){
    asset_group* Grp = &Assets->Groups[Group];
    
    u32 Result = 0;
    
    if(!DLIST_FREE_IS_EMPTY(Grp->Sentinel, Next)){
        Result = Grp->Sentinel.Next->ID;
    }
    
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

inline asset_file_source* AllocateFileSource(assets* Assets){
    if(DLIST_FREE_IS_EMPTY(Assets->FileSourceFree, Next)){
        const int Count = 128;
        asset_file_source* Pool = PushArray(Assets->Memory, asset_file_source, Count);
        
        for(int I = 0; I < Count; I++){
            asset_file_source* Elem = &Pool[I];
            
            DLIST_INSERT_BEFORE_SENTINEL(Elem, Assets->FileSourceFree, Next, Prev);
        }
        
    }
    
    asset_file_source* Result = Assets->FileSourceFree.Next;
    
    DLIST_REMOVE_ENTRY(Result, Next, Prev);
    DLIST_INSERT_BEFORE_SENTINEL(Result, Assets->FileSourceUse, Next, Prev);
    
    return(Result);
}

void LoadAsset(assets* Assets, asset* Asset){
    asset_header* Header = &Asset->Header;
    
    // NOTE(Dima): Loading data
    u32 DataSize = Header->TotalDataSize;
    // TODO(Dima): Change this
    void* Data = malloc(DataSize);
    
    char* FilePath = Asset->FileSource->FileDescription.FullPath;
    
    b32 ReadSucceeded = platform.FileOffsetRead(FilePath,
                                                Asset->OffsetToData,
                                                DataSize,
                                                Data);
    
    ASSERT(ReadSucceeded);
    
    switch(Asset->Type){
        case AssetType_Bitmap:{
            // NOTE(Dima): Allocating asset value
            GET_ASSET_PTR_MEMBER(Asset, bmp_info) = (bmp_info*)malloc(sizeof(bmp_info));
            
            bmp_info* Result = GET_ASSET_PTR_MEMBER(Asset, bmp_info);
            asset_bitmap* Src = &Header->Bitmap;
            
            // NOTE(Dima): Initializing bitmap
            AllocateBitmapInternal(Result, Src->Width, Src->Height, Data);
            
            // NOTE(Dima): Adding to atlas if needed
            if(Src->BakeToAtlas){
                AddBitmapToAtlas(&Assets->MainLargeAtlas, Result);
            }
        }break;
        
        case AssetType_Glyph:{
            
        }break;
        
        case AssetType_BitmapArray:{
            
        }break;
        
        case AssetType_Mesh:{
            
        }break;
        
        case AssetType_Sound:{
            
        }break;
        
        case AssetType_Font:{
            // NOTE(Dima): Allocating asset value
            GET_ASSET_PTR_MEMBER(Asset, font_info) = (font_info*)malloc(sizeof(font_info));
            
            font_info* Result = GET_ASSET_PTR_MEMBER(Asset, font_info);
            asset_font* Src = &Header->Font;
            
            int* Mapping = (int*)((u8*)Data + Header->Font.DataOffsetToMapping);
            float* KerningPairs = (float*)((u8*)Data + Header->Font.DataOffsetToKerning);
            
            u32 MappingSize = Header->Font.MappingSize;
            u32 KerningSize = Header->Font.KerningSize;
            
            ASSERT(MappingSize == sizeof(float) * FONT_INFO_MAX_GLYPH_COUNT);
            
            Result->AscenderHeight = Header->Font.AscenderHeight;
            Result->DescenderHeight = Header->Font.DescenderHeight;
            Result->LineGap = Header->Font.LineGap;
            Result->GlyphCount = Header->Font.GlyphCount;
            
            // NOTE(Dima): Copy mapping
            for(int I = 0; I < FONT_INFO_MAX_GLYPH_COUNT; I++){
                Result->Codepoint2Glyph[I] = Mapping[I];
            }
            
            // NOTE(Dima): Setting kerning
            Result->KerningPairs = KerningPairs;
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
        CurBlock->BlockAssets = PushArray(Assets->Memory, asset, MAX_ASSETS_IN_ASSET_BLOCK);
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

void InitAssets(assets* Assets){
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // NOTE(Dima): Memory region is already initialized
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    // NOTE(Dima): Large atlas initialization
    Assets->MainLargeAtlas = InitAtlas(Assets->Memory, 1024);
    
    // NOTE(Dima): Init asset files sources
    DLIST_REFLECT_PTRS(Assets->FileSourceUse, Next, Prev);
    DLIST_REFLECT_PTRS(Assets->FileSourceFree, Next, Prev);
    
    mem_region AssetInitMem = {};
    
    // NOTE(Dima): Temp initialization of asset families
    for(int FamIndex = 0;
        FamIndex < GameAsset_Count;
        FamIndex++)
    {
        asset_group* Grp = &Assets->Groups[FamIndex];
        
        Grp->InGroupAssetCount = 0;
        DLIST_REFLECT_PTRS(Grp->Sentinel, Next, Prev);
    }
    
    // NOTE(Dima): Loading from asset files
    platform.OpenFilesBegin("../Data/", "*ja");
    
    platform_file_desc FileDesc;
    
    while(platform.OpenNextFile(&FileDesc)){
        // NOTE(Dima): Allocating and setting file source
        asset_file_source* FileSource = AllocateFileSource(Assets);
        FileSource->FileDescription = FileDesc;
        
        char* FileFullPath = FileSource->FileDescription.FullPath;
        
        asset_file_header FileHeader;
        b32 ReadFileResult = platform.FileOffsetRead(FileFullPath, 
                                                     0, sizeof(asset_file_header), 
                                                     &FileHeader);
        
        Assert(ReadFileResult);
        
        b32 HeaderIsEqual =
            FileHeader.FileHeader[0] == 'J' &&
            FileHeader.FileHeader[1] == 'A' &&
            FileHeader.FileHeader[2] == 'S' &&
            FileHeader.FileHeader[3] == 'S';
        
        u32 FileVersion = FileHeader.Version;
        u32 EngineFileVersion = GetVersionInt(ASSET_FILE_VERSION_MAJOR,
                                              ASSET_FILE_VERSION_MINOR);
        
        // NOTE(Dima): Some checking
        Assert(HeaderIsEqual);
        Assert(FileVersion == EngineFileVersion);
        Assert(FileHeader.GroupsCount == GameAsset_Count);
        
        asset_file_group *FileGroups = PushArray(&AssetInitMem,
                                                 asset_file_group,
                                                 GameAsset_Count);
        
        // NOTE(Dima): Reading groups
        Assert(FileHeader.GroupsByteOffset == sizeof(asset_file_header));
        b32 ReadGroupsResult = platform.FileOffsetRead(FileFullPath,
                                                       FileHeader.GroupsByteOffset,
                                                       sizeof(asset_file_group) * 
                                                       FileHeader.GroupsCount,
                                                       FileGroups);
        Assert(ReadGroupsResult);
        
        // NOTE(Dima): Reading lines offsets
        u32 FileAssetCount = FileHeader.EffectiveAssetsCount;
        u32* AssetLinesOffsets = 0;
        
        if(FileAssetCount){
            AssetLinesOffsets = PushArray(&AssetInitMem, u32, FileAssetCount);
            b32 ReadOffsetsRes = platform.FileOffsetRead(FileFullPath,
                                                         FileHeader.LinesOffsetsByteOffset,
                                                         FileAssetCount * sizeof(u32),
                                                         AssetLinesOffsets);
            
            Assert(ReadOffsetsRes);
        }
        
        // NOTE(Dima): Reading assets
        for(int FileGroupIndex = 0; 
            FileGroupIndex < GameAsset_Count; 
            FileGroupIndex++)
        {
            asset_file_group* FileGrp = FileGroups + FileGroupIndex;
            asset_group* ToGroup = Assets->Groups + FileGroupIndex;
            
            // NOTE(Dima): Iterating through regions in file
            for(int RegionIndex = 0;
                RegionIndex < FileGrp->Count;
                RegionIndex++)
            {
                asset_file_group_region* Reg = FileGrp->Regions + RegionIndex;
                
                u32 FirstFileAssetIndex = Reg->FirstAssetIndex;
                u32 OnePastLastFileAssetIndex = FirstFileAssetIndex + Reg->AssetCount;
                
                // NOTE(Dima): Iterating through region assets
                for(int FileAssetIndex = FirstFileAssetIndex;
                    FileAssetIndex < OnePastLastFileAssetIndex;
                    FileAssetIndex++)
                {
                    // NOTE(Dima): Reading asset header
                    asset_header AssetHeader;
                    
                    u32 LineOffset = AssetLinesOffsets[FileAssetIndex - 1];
                    
                    b32 ReadAssetHeader = platform.FileOffsetRead(
                        FileFullPath,
                        LineOffset,
                        sizeof(asset_header),
                        &AssetHeader);
                    
                    Assert(ReadAssetHeader);
                    
                    // NOTE(Dima): Allocating asset
                    asset* NewAsset = AllocateAsset(Assets);
                    
                    NewAsset->State = AssetState_Unloaded;
                    NewAsset->Header = AssetHeader;
                    NewAsset->FileSource = FileSource;
                    NewAsset->Type = AssetHeader.AssetType;
                    
                    DLIST_INSERT_BEFORE_SENTINEL(NewAsset, ToGroup->Sentinel, Next, Prev);
                    ToGroup->InGroupAssetCount++;
                    
                    u32 DataOffsetInFile = LineOffset + AssetHeader.LineDataOffset;
                    NewAsset->OffsetToData = DataOffsetInFile;
                    
                    LoadAsset(Assets, NewAsset);
                }
            }
        }
        
        FreeNoDealloc(&AssetInitMem);
    }
    
    platform.OpenFilesEnd();
    
    Free(&AssetInitMem);
    
    
#if 0    
#define TEMP_INIT_FAM_ASSET(fam_id, data_type, value) \
    {\
        asset_group* Grp = &Assets->Groups[fam_id];\
        asset* Asset = GetAssetByID(Assets, Grp->AssetID);\
        Asset->Type = AssetType_Type_##data_type;\
        Asset->Data_##data_type = value;\
        Asset->Ptr_##data_type = &Asset->Data_##data_type;\
        FinalAssetLoading(Assets, Asset);}
    
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
#endif
    
}