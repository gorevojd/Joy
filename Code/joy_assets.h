#ifndef JOY_ASSETS_H
#define JOY_ASSETS_H

#include <atomic>

#include "joy_asset_types.h"
#include "joy_asset_util.h"
#include "joy_asset_ids.h"

#define ASSET_IMPORT_IMMEDIATE true
#define ASSET_IMPORT_DEFERRED false

struct asset_file_source{
    platform_file_desc FileDescription;
    
    u32 IntegrationBaseID;
    
    asset_file_source* Next;
    asset_file_source* Prev;
};

#define MAX_TAGS_PER_ASSET 4

struct asset{
    u32 ID;
    u32 Type;
    
    std::atomic_uint State;
    
    asset_tag_header Tags[MAX_TAGS_PER_ASSET];
    int TagCount;
    
    // NOTE(Dima): Asset header
    asset_header Header;
    
    // NOTE(Dima): Find by tags context
    float FindWeight;
    
    // NOTE(Dima): File asset source
    asset_file_source* FileSource;
    u32 OffsetToData;
    
    //void* TempData;
    
    // NOTE(Dima): Asset data memory entry
    mem_entry* DataMemoryEntry;
    
    // NOTE(Dima): In list stuff
    asset* Next;
    asset* Prev;
    
    mem_layer_entry* TypeMemEntry;
    
    // NOTE(Dima): Data
    union{
        ASSET_PTR_MEMBER(font_info);
        ASSET_PTR_MEMBER(bmp_info);
        ASSET_PTR_MEMBER(array_info);
        ASSET_PTR_MEMBER(mesh_info);
        ASSET_PTR_MEMBER(material_info);
        ASSET_PTR_MEMBER(glyph_info);
        ASSET_PTR_MEMBER(sound_info);
        ASSET_PTR_MEMBER(model_info);
        ASSET_PTR_MEMBER(skeleton_info);
        ASSET_PTR_MEMBER(node_info);
        ASSET_PTR_MEMBER(node_animation);
        ASSET_PTR_MEMBER(animation_clip);
    }Data;
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
    asset** PointersToAssets;
};

#define MAX_ASSETS_IN_ASSET_BLOCK 4096
#define MAX_ASSET_BLOCKS_COUNT 1024

struct asset_block{
    asset* BlockAssets;
    u32 InBlockCount;
};

// NOTE(dima): Bitmaps are stored in gamma-corrected premultiplied alpha format
struct assets{
    mem_region* Memory;
    layered_mem LayeredMemory;
    
    task_data_pool ImportTasksPool;
    task_data_pool ImportTaskPoolMainThread;
    
    random_generation Random;
    
    Asset_Atlas MainLargeAtlas;
    
    // NOTE(Dima): Asset files sources
    asset_file_source FileSourceUse;
    asset_file_source FileSourceFree;
    
    // NOTE(Dima): Memory entries
    mem_box MemBox;
    
    // NOTE(Dima): Asset groups
    asset_group Groups[GameAsset_Count];
    
    // NOTE(Dima): Actual asset storage
    asset_block AssetBlocks[MAX_ASSET_BLOCKS_COUNT];
    int CurrentBlockIndex;
};

struct parsed_asset_id{
    u32 InBlockIndex;
    u32 BlockIndex;
};

inline parsed_asset_id ParseAssetID(u32 ID){
    parsed_asset_id Result = {};
    
    Result.InBlockIndex = ID & 0xFFF;
    Result.BlockIndex = (ID >> 12) & 0xFFFFF;
    
    return(Result);
}

inline u32 RestoreAssetID(u32 BlockIndex, u32 InBlockIndex){
    u32 Result = 
        (InBlockIndex & 0xFFF) | 
        ((BlockIndex & 0xFFFFF) << 12);
    
    return(Result);
}

inline b32 PotentiallyLoadedAsset(asset* Asset, b32 Immediate){
    b32 Result = (Asset->State == AssetState_Loaded) || Immediate;
    
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
u32 GetRandom(assets* Assets, u32 Group);
u32 GetBestByTags(assets* Assets, 
                  u32 Group, 
                  u32* TagTypes, 
                  asset_tag_value* TagValues, 
                  int TagsCount);

void ImportAsset(assets* Assets, asset* Asset, b32 Immediate);


inline void* LoadAssetTypeInternal(assets* Assets, 
                                   asset_id ID, 
                                   u32 CompareAssetType,
                                   b32 Immediate)
{
    asset* Asset = GetAssetByID(Assets, ID);
    ASSERT(Asset->Type == CompareAssetType);
    
    ImportAsset(Assets, Asset, Immediate);
    
    void* Result = 0;
    if(PotentiallyLoadedAsset(Asset, Immediate)){
        Result = (void*)(*((size_t*)(&Asset->Data)));
    }
    
    return(Result);
}

#define LOAD_ASSET(data_type, type, assets, id, immediate) \
(data_type*)LoadAssetTypeInternal(assets, id, type, immediate)

array_info* LoadArray(assets* Assets,
                      u32 ArrayID);

bmp_info* LoadBmp(assets* Assets,
                  u32 BmpID,
                  b32 Immediate);

font_info* LoadFont(assets* Assets,
                    u32 FontID,
                    b32 Immediate);

mesh_info* LoadMesh(assets* Assets,
                    u32 MeshID,
                    b32 Immediate);

material_info* LoadMaterial(assets* Assets,
                            u32 MaterialID,
                            b32 Immediate);

model_info* LoadModel(assets* Assets,
                      u32 ModelID,
                      b32 Immediate);

skeleton_info* LoadSkeleton(assets* Assets,
                            u32 SkeletonID,
                            b32 Immediate);

animation_clip* LoadAnimationClip(assets* Assets,
                                  u32 AnimID,
                                  b32 Immediate);

node_animation* LoadNodeAnim(assets* Assets,
                             u32 NodeAnimID,
                             b32 Immediate);

#endif