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

struct asset{
    mem_entry* DataMemoryEntry;
    
    u32 ID;
    u32 Type;
    
    std::atomic_uint State;
    
    asset* NextInFamily;
    asset* PrevInFamily;
    
    union{
        ASSET_VALUE_MEMBER(font_info);
        ASSET_VALUE_MEMBER(bmp_info);
        ASSET_VALUE_MEMBER(mesh_info);
        ASSET_VALUE_MEMBER(glyph_info);
        ASSET_VALUE_MEMBER(sound_info);
    };
    
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

struct asset_family{
    u32 AssetID;
};

#define MAX_ASSETS_IN_ASSET_BLOCK 8
#define MAX_ASSET_BLOCKS_COUNT 1024

struct asset_block{
    asset* BlockAssets;
    u32 InBlockCount;
};

// NOTE(dima): Bitmaps are stored in gamma-corrected premultiplied alpha format
struct assets{
    mem_region* Region;
    
    Asset_Atlas MainLargeAtlas;
    
    // NOTE(Dima): Memory entries
    mem_box MemBox;
    
    // NOTE(Dima): Assets
    asset_family Families[GameAsset_Count];
    
    int CurrentBlockIndex;
    asset_block AssetBlocks[MAX_ASSET_BLOCKS_COUNT];
};

inline asset* GetAssetByID(assets* Assets, u32 ID){
    int InBlockIndex = ID & 0xFFFF;
    int AssetBlockIndex = (ID >> 16) & 0xFFFF;
    
    asset_block* Block = &Assets->AssetBlocks[AssetBlockIndex];
    
    ASSERT(Block->BlockAssets);
    
    asset* Result = &Block->BlockAssets[InBlockIndex];
    
    return(Result);
}

void InitAssets(assets* Assets);
u32 GetFirstInFamily(assets* Assets, u32 Family);

#endif