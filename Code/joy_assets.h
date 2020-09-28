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
    int EntryIndex;
    
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
    
    mem_layer_entry* TypeMemEntry;
    
    // NOTE(Dima): Data
    union{
        void* Stub;
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

struct asset_slot{
    asset* Asset;
    
    asset_slot* Next;
    asset_slot* Prev;
    
    asset_slot* NextAlloc;
    asset_slot* PrevAlloc;
};

struct added_asset{
    asset* Asset;
    u32 ID;
};

struct asset_id_range{
    u32 StartID;
    int Count;
};

struct asset_group_context
{
    char* GroupName;
    
    int Count;
    u32 NameHash;
};

struct asset_entry{
    int InEntryAssetCount;
    asset_slot Sentinel;
    asset** PointersToAssets;
};

#define MAX_ASSETS_IN_ASSET_BLOCK 4096
#define MAX_ASSET_BLOCKS_COUNT 256

struct asset_block{
    asset* BlockAssets;
    u32 InBlockCount;
};

// NOTE(dima): Bitmaps are stored in gamma-corrected premultiplied alpha format
struct asset_system
{
    mem_arena* Memory;
    layered_mem LayeredMemory;
    
    Asset_Atlas MainLargeAtlas;
    
    // NOTE(Dima): Asset files sources
    asset_file_source FileSourceUse;
    asset_file_source FileSourceFree;
    
    // NOTE(Dima): Asset groups
    // TODO(Dima): Allocate memory for those
    asset_entry Entries[AssetEntry_Count];
    asset_entry TaggedEntries[AssetTag_Count][AssetEntry_Count];
    
    // NOTE(Dima): Actual asset storage
    asset_block AssetBlocks[MAX_ASSET_BLOCKS_COUNT];
    int CurrentBlockIndex;
    
    task_data_pool ImportTasksPool;
    task_data_pool ImportTaskPoolMainThread;
    
    random_generation Random;
    
    asset_slot UseSlot;
    asset_slot FreeSlot;
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

inline asset* GetAssetByID(asset_system* Assets, u32 ID){
    
    parsed_asset_id ParsedID = ParseAssetID(ID);
    
    asset_block* Block = &Assets->AssetBlocks[ParsedID.BlockIndex];
    
    ASSERT(Block->BlockAssets);
    
    asset* Result = &Block->BlockAssets[ParsedID.InBlockIndex];
    
    return(Result);
}

void InitAssets(asset_system* Assets);
u32 GetFirst(asset_system* Assets, u32 Family);
u32 GetRandom(asset_system* Assets, u32 Group);
u32 GetBestByTags(asset_system* Assets, 
                  u32 Group, 
                  u32* TagTypes, 
                  asset_tag_value* TagValues, 
                  int TagsCount);

void ImportAsset(asset_system* Assets, asset* Asset, b32 Immediate);

// NOTE(Dima): This functions returns asset only if it was loaded.
// NOTE(Dima): Otherwise it returns NULL
inline void* LoadAssetTypeInternal(asset_system* Assets, 
                                   asset_id ID, 
                                   u32 CompareAssetType,
                                   b32 Immediate)
{
    asset* Asset = GetAssetByID(Assets, ID);
    
    void* Result = 0;
    if(Asset->Type != AssetType_None){
        ASSERT(Asset->Type == CompareAssetType);
        
        ImportAsset(Assets, Asset, Immediate);
        
        if(PotentiallyLoadedAsset(Asset, Immediate)){
            Result = Asset->Data.Stub;
        }
    }
    
    return(Result);
}

inline void* LoadAssetTypeRawInternal(asset_system* Assets, 
                                      asset_id ID, 
                                      u32 CompareAssetType)
{
    asset* Asset = GetAssetByID(Assets, ID);
    ASSERT(Asset->Type == CompareAssetType);
    
    void* Result = Asset->Data.Stub;
    
    return(Result);
}

#define GET_ASSET_DATA_BY_ID(data_type, type, assets, id) \
(data_type*)LoadAssetTypeRawInternal(assets, id, type)

#define LOAD_ASSET(data_type, type, assets, id, immediate) \
(data_type*)LoadAssetTypeInternal(assets, id, type, immediate)

array_info* LoadArray(asset_system* Assets,
                      u32 ArrayID);

bmp_info* LoadBmp(asset_system* Assets,
                  u32 BmpID,
                  b32 Immediate);

font_info* LoadFont(asset_system* Assets,
                    u32 FontID,
                    b32 Immediate);

mesh_info* LoadMesh(asset_system* Assets,
                    u32 MeshID,
                    b32 Immediate);

material_info* LoadMaterial(asset_system* Assets,
                            u32 MaterialID,
                            b32 Immediate);

model_info* LoadModel(asset_system* Assets,
                      u32 ModelID,
                      b32 Immediate);

skeleton_info* LoadSkeleton(asset_system* Assets,
                            u32 SkeletonID,
                            b32 Immediate);

animation_clip* LoadAnimationClip(asset_system* Assets,
                                  u32 AnimID,
                                  b32 Immediate);

node_animation* LoadNodeAnim(asset_system* Assets,
                             u32 NodeAnimID,
                             b32 Immediate);

#endif