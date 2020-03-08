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

inline asset_id FileToIntegratedID(asset_file_source* Source, u32 FileID){
    asset_id Result = FileID - 1 + Source->IntegrationBaseID;
    
    return(Result);
}

void* AllocateAssetType(assets* Assets, asset* Asset, u32 AssetTypeSize){
    Asset->TypeMemEntry = AllocateMemLayerEntry(
        &Assets->LayeredMemory, AssetTypeSize);
    
    ASSERT(Asset->TypeMemEntry);
    
    void* Result = Asset->TypeMemEntry->Data;
    
    return(Result);
}

#define ALLOC_ASS_PTR_MEMBER(type) (GET_ASSET_PTR_MEMBER(Asset, type) = (type*)AllocateAssetType(Assets, Asset, sizeof(type)))

void LoadAsset(assets* Assets, asset* Asset){
    asset_header* Header = &Asset->Header;
    asset_file_source* FileSource = Asset->FileSource;
    
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
            Asset->TypeMemEntry = AllocateMemLayerEntry(
                &Assets->LayeredMemory,sizeof(bmp_info));
            
            ASSERT(Asset->TypeMemEntry);
            
            bmp_info* Result = ALLOC_ASS_PTR_MEMBER(bmp_info);
            
            asset_bitmap* Src = &Header->Bitmap;
            
            *Result = {};
            
            // NOTE(Dima): Initializing bitmap
            AllocateBitmapInternal(Result, Src->Width, Src->Height, Data);
            
            // NOTE(Dima): Adding to atlas if needed
            if(Src->BakeToAtlas){
                AddBitmapToAtlas(&Assets->MainLargeAtlas, Result);
            }
        }break;
        
        case AssetType_Glyph:{
            glyph_info* Result = ALLOC_ASS_PTR_MEMBER(glyph_info);
            
            asset_glyph* Src = &Header->Glyph;
            
            Result->BitmapID = FileToIntegratedID(FileSource, Src->BitmapID);
            
            Result->Codepoint = Src->Codepoint;
            Result->Width = Src->BitmapWidth;
            Result->Height = Src->BitmapHeight;
            Result->WidthOverHeight = Src->BitmapWidthOverHeight;
            Result->XOffset = Src->XOffset;
            Result->YOffset = Src->YOffset;
            Result->Advance = Src->Advance;
            Result->LeftBearingX = Src->LeftBearingX;
        }break;
        
        case AssetType_BitmapArray:{
            bmp_array_info* Result = ALLOC_ASS_PTR_MEMBER(bmp_array_info);
            asset_bitmap_array* Src = &Header->BmpArray;
            
            Result->FirstBmpID = Src->FirstBmpID;
            Result->Count = Src->Count;
        }break;
        
        case AssetType_Mesh:{
            mesh_info* Result = ALLOC_ASS_PTR_MEMBER(mesh_info);
            asset_mesh* Src = &Header->Mesh;
            
            *Result = {};
            
            Result->VerticesCount = Src->VerticesCount;
            Result->IndicesCount = Src->IndicesCount;
            Result->MeshType = Src->MeshType;
            
            // NOTE(Dima): Load mesh data
            u32 VertSize = Header->Mesh.DataVerticesSize;
            u32 IndiSize = Header->Mesh.DataIndicesSize;
            
            void* Vertices = (u8*)Data + Header->Mesh.DataOffsetToVertices;
            u32* Indices = (u32*)((u8*)Data + Header->Mesh.DataOffsetToIndices);
            
            Result->Vertices = Vertices;
            Result->Indices = Indices;
        }break;
        
        case AssetType_Sound:{
            
        }break;
        
        case AssetType_Font:{
            font_info* Result = ALLOC_ASS_PTR_MEMBER(font_info);
            asset_font* Src = &Header->Font;
            
            *Result = {};
            
            int* Mapping = (int*)((u8*)Data + Header->Font.DataOffsetToMapping);
            float* KerningPairs = (float*)((u8*)Data + Header->Font.DataOffsetToKerning);
            u32* GlyphIDs = (u32*)((u8*)Data + Header->Font.DataOffsetToIDs);
            
            u32 MappingSize = Header->Font.MappingSize;
            u32 KerningSize = Header->Font.KerningSize;
            u32 IDsSize = Header->Font.IDsSize;
            
            ASSERT(MappingSize == sizeof(float) * FONT_INFO_MAX_GLYPH_COUNT);
            
            Result->AscenderHeight = Header->Font.AscenderHeight;
            Result->DescenderHeight = Header->Font.DescenderHeight;
            Result->LineGap = Header->Font.LineGap;
            Result->GlyphCount = Header->Font.GlyphCount;
            
            // NOTE(Dima): Copy glyph IDs
            Result->GlyphIDs = GlyphIDs;
            
            // NOTE(Dima): Fixing Glyph IDs
            for(int GlyphIndex = 0;
                GlyphIndex < Header->Font.GlyphCount;
                GlyphIndex++)
            {
                Result->GlyphIDs[GlyphIndex] = FileToIntegratedID(
                    FileSource,
                    Result->GlyphIDs[GlyphIndex]);
            }
            
            // NOTE(Dima): Copy mapping
            for(int I = 0; I < FONT_INFO_MAX_GLYPH_COUNT; I++){
                Result->Codepoint2Glyph[I] = Mapping[I];
            }
            
            // NOTE(Dima): Setting kerning
            Result->KerningPairs = KerningPairs;
        }break;
    }
}

inline asset* AllocateAsset(assets* Assets, asset_file_source* FileSource, u32 FileID)
{
    u32 ResultIntegratedID = FileToIntegratedID(FileSource, FileID);
    asset* Result = GetAssetByID(Assets, ResultIntegratedID);
    
    Result->Type = AssetType_None;
    Result->State = AssetState_Unloaded;
    Result->ID = ResultIntegratedID;
    Result->FileSource = FileSource;
    Result->DataMemoryEntry = 0;
    Result->TypeMemEntry = 0;
    
    return(Result);
}

void InitAssets(assets* Assets){
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // NOTE(Dima): Memory region is already initialized
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    // NOTE(Dima): Large atlas initialization
    Assets->MainLargeAtlas = InitAtlas(Assets->Memory, 1024);
    
    // NOTE(Dima): Init asset layered memory to allocate asset types
    u32 LayersSizes[] = {64, 128, 256, 512, 1024};
    u32 LayersSizesCount = ARRAY_COUNT(LayersSizes);
    InitLayeredMem(&Assets->LayeredMemory, 
                   Assets->Memory, 
                   LayersSizes, 
                   LayersSizesCount);
    
    // NOTE(Dima): Init asset files sources
    DLIST_REFLECT_PTRS(Assets->FileSourceUse, Next, Prev);
    DLIST_REFLECT_PTRS(Assets->FileSourceFree, Next, Prev);
    
    mem_region AssetInitMem = {};
    
    // NOTE(Dima): Init first null asset
    Assets->AssetBlocks[0].BlockAssets = PushArray(Assets->Memory, asset, MAX_ASSETS_IN_ASSET_BLOCK);
    Assets->AssetBlocks[0].InBlockCount = 1;
    
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
        FileSource->IntegrationBaseID = 0;
        
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
        
        // NOTE(Dima): Getting needed asset chunk
        asset_block* PrevBlock = &Assets->AssetBlocks[Assets->CurrentBlockIndex];
        asset_block* CurBlock = PrevBlock;
        
        if(PrevBlock->InBlockCount + FileAssetCount >= MAX_ASSETS_IN_ASSET_BLOCK){
            ++Assets->CurrentBlockIndex;
            ASSERT(Assets->CurrentBlockIndex < MAX_ASSET_BLOCKS_COUNT);
            
            CurBlock = &Assets->AssetBlocks[Assets->CurrentBlockIndex];
            CurBlock->InBlockCount = 0;
        }
        
        int AssetBlockIndex = Assets->CurrentBlockIndex;
        
        // NOTE(Dima): If block assets are not allocated yet
        if(!CurBlock->BlockAssets){
            // NOTE(Dima): Allocating
            CurBlock->BlockAssets = PushArray(Assets->Memory, asset, MAX_ASSETS_IN_ASSET_BLOCK);
        }
        
        u32 IntegrationBaseID = RestoreAssetID(
            AssetBlockIndex, CurBlock->InBlockCount);
        
        // NOTE(Dima): Setting integration base ID
        FileSource->IntegrationBaseID = IntegrationBaseID;
        
        // NOTE(Dima): Settings new Assets Count
        CurBlock->InBlockCount += FileAssetCount;
        
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
                    asset* NewAsset = AllocateAsset(Assets, FileSource, FileAssetIndex);
                    
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
    
}