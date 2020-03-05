#ifndef JOY_ASSETS_H
#define JOY_ASSETS_H

#include <atomic>

#include "joy_types.h"
#include "joy_defines.h"
#include "joy_asset_types.h"
#include "joy_asset_util.h"
#include "joy_platform.h"
#include "joy_memory.h"

#include "joy_asset_ids.h"
#include "joy_data_structures.h"

struct asset_file_source{
    platform_file_desc FileDescription;
    
    u32 IntegrationBaseID;
    b32 IntegrationBaseIDInitialized;
    
    asset_file_source* Next;
    asset_file_source* Prev;
};

struct asset{
    u32 ID;
    u32 Type;
    
    std::atomic_uint State;
    
    // NOTE(Dima): Asset header
    asset_header Header;
    
    // NOTE(Dima): File asset source
    asset_file_source* FileSource;
    u32 OffsetToData;
    
    // NOTE(Dima): Asset data memory entry
    mem_entry* DataMemoryEntry;
    
    // NOTE(Dima): In list stuff
    asset* Next;
    asset* Prev;
    
    // NOTE(Dima): Data
    union{
        ASSET_PTR_MEMBER(font_info);
        ASSET_PTR_MEMBER(bmp_info);
        ASSET_PTR_MEMBER(mesh_info);
        ASSET_PTR_MEMBER(glyph_info);
        ASSET_PTR_MEMBER(sound_info);
    };
};

struct added_asset{
    asset* Asset;
    u32 ID;
};

struct asset_id_range{
    u32 StartID;
    int Count;
};

struct asset_group{
    int InGroupAssetCount;
    asset Sentinel;
};

#define MAX_ASSETS_IN_ASSET_BLOCK 8
#define MAX_ASSET_BLOCKS_COUNT 1024

struct asset_block{
    asset* BlockAssets;
    u32 InBlockCount;
};

// NOTE(dima): Bitmaps are stored in gamma-corrected premultiplied alpha format
struct assets{
    mem_region* Memory;
    
    Asset_Atlas MainLargeAtlas;
    
    // NOTE(Dima): Asset files sources
    asset_file_source FileSourceUse;
    asset_file_source FileSourceFree;
    
    // NOTE(Dima): Memory entries
    mem_box MemBox;
    
    // NOTE(Dima): Assets
    asset_group Groups[GameAsset_Count];
    
    int CurrentBlockIndex;
    asset_block AssetBlocks[MAX_ASSET_BLOCKS_COUNT];
};

struct parsed_asset_id{
    int InBlockIndex;
    int BlockIndex;
};

inline parsed_asset_id ParseAssetID(u32 ID2Parse){
    parsed_asset_id Result = {};
    
    Result.InBlockIndex = ID2Parse & 0xFFFF;
    Result.BlockIndex = (ID2Parse >> 16) & 0xFFFF;
    
    return(Result);
}

inline u32 RecoverAssetID(u32 BlockIndex, u32 InBlockIndex){
    u32 ResultAssetID = 
        (InBlockIndex & 0xFFFF) | 
        ((BlockIndex & 0xFFFF) << 16);
    
    return(ResultAssetID);
}

inline u32 SumAssetID(u32 ID, int AddValue){
    // NOTE(Dima): I guess we can cheat here and just add as always
    u32 Result = ID + AddValue;
    
    if((int)ID + AddValue < 0){
        INVALID_CODE_PATH;
        Result = 0;
    }
    
    return(Result);
}


inline asset* GetAssetByID(assets* Assets, u32 ID){
    parsed_asset_id ParsedID = ParseAssetID(ID);
    
    asset_block* Block = &Assets->AssetBlocks[ParsedID.BlockIndex];
    
    ASSERT(Block->BlockAssets);
    
    asset* Result = &Block->BlockAssets[ParsedID.InBlockIndex];
    
    return(Result);
}

void InitAssets(assets* Assets);
u32 GetFirst(assets* Assets, u32 Family);

#endif