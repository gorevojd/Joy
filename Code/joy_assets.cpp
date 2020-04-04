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
    
    if(Grp->InGroupAssetCount){
        Result = Grp->PointersToAssets[0]->ID;
    }
    
    return(Result);
}

u32 GetRandom(assets* Assets, u32 Group){
    asset_group* Grp = &Assets->Groups[Group];
    
    u32 Result = 0;
    
    if(Grp->InGroupAssetCount){
        int RandomIndex = GetRandomIndex(&Assets->Random, 
                                         Grp->InGroupAssetCount);
        
        Result = Grp->PointersToAssets[RandomIndex]->ID;
    }
    
    return(Result);
}

u32 GetBestByTags(assets* Assets, 
                  u32 Group, 
                  u32* TagTypes, 
                  asset_tag_value* TagValues, 
                  int TagsCount)
{
    asset_group* Grp = &Assets->Groups[Group];
    
    u32 Result = 0;
    
    /*
TODO(Dima): Ideas to make it faster:
*/
    
    if(Grp->InGroupAssetCount){
        int BestIndex = 0;
        float BestWeight = 0.0f;
        
        for(int GrpAssetIndex = 0; 
            GrpAssetIndex < Grp->InGroupAssetCount;
            GrpAssetIndex++)
        {
            asset* Asset = Grp->PointersToAssets[GrpAssetIndex];
            
            b32 ShouldExitAssetLoop = false;
            
            float Weight = 0.0f;
            
            // TODO(Dima): Speed up this part where we find corresponding tags
            for(int MatchTagIndex = 0;
                MatchTagIndex < TagsCount;
                MatchTagIndex++)
            {
                asset_tag_value* Value = &TagValues[MatchTagIndex];
                u32 TagType = TagTypes[MatchTagIndex];
                
                b32 ShouldExitMatchLoop = false;
                
                for(int TagIndex = 0; TagIndex < Asset->TagCount; TagIndex++)
                {
                    asset_tag_header* Tag = &Asset->Tags[TagIndex];
                    
                    if(Tag->Type == TagType){
                        // NOTE(Dima): We found needed tag
                        switch(Tag->ValueType){
                            case AssetTagValue_Float:{
                                float Diff = abs(Tag->Value.Value_Float - Value->Value_Float);
                                
                                Weight += (1.0f - Diff);
                            }break;
                            
                            case AssetTagValue_Int:{
                                int Diff = Abs(Tag->Value.Value_Int - Value->Value_Int);
                                
                                Weight += 1.0f - ((float)(Diff) * 0.01f);
                            }break;
                            
                            case AssetTagValue_Empty:{
                                BestIndex = GrpAssetIndex;
                                
                                ShouldExitMatchLoop = true;
                                ShouldExitAssetLoop = true;
                                break;
                            }break;
                        }
                    }
                }
                
                if(ShouldExitMatchLoop){
                    break;
                }
            }
            
            if(Weight > BestWeight){
                BestWeight = Weight;
                BestIndex = GrpAssetIndex;
            }
            
            if(ShouldExitAssetLoop){
                break;
            }
        }
        
        Result = Grp->PointersToAssets[BestIndex]->ID;
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
    asset_id Result;
    
    if(FileID){
        Result = FileID - 1 + Source->IntegrationBaseID;
    }
    else{
        Result = 0;
    }
    
    return(Result);
}

inline void IntegrateIDs(asset_id* IDsToIntegrate, 
                         int Count, 
                         asset_file_source* Source)
{
    for(int Index = 0;
        Index < Count;
        Index++)
    {
        IDsToIntegrate[Index] = FileToIntegratedID(Source, IDsToIntegrate[Index]);
    }
}

void* AllocateAssetType(assets* Assets, asset* Asset, void** Type, u32 AssetTypeSize){
    Asset->TypeMemEntry = AllocateMemLayerEntry(
        &Assets->LayeredMemory, AssetTypeSize);
    
    ASSERT(Asset->TypeMemEntry);
    
    void* Result = Asset->TypeMemEntry->Data;
    
    Platform.MemZeroRaw(Result, AssetTypeSize);
    
    *Type = Result;
    
    return(Result);
}

void ImportAssetDirectly(assets* Assets, 
                         asset* Asset, 
                         void* Data, 
                         u64 DataSize)
{
    asset_header* Header = &Asset->Header;
    asset_file_source* FileSource = Asset->FileSource;
    
    char* FilePath = Asset->FileSource->FileDescription.FullPath;
    
    b32 ReadSucceeded = Platform.FileOffsetRead(FilePath,
                                                Asset->OffsetToData,
                                                DataSize,
                                                Data);
    
    ASSERT(ReadSucceeded);
    
    switch(Asset->Type){
        case AssetType_Bitmap:{
            bmp_info* Result = GET_ASSET_PTR_MEMBER(Asset, bmp_info);
            asset_bitmap* Src = &Header->Bitmap;
            
            // NOTE(Dima): Initializing bitmap
            AllocateBitmapInternal(Result, Src->Width, Src->Height, Data);
        }break;
        
        case AssetType_Glyph:{
            glyph_info* Result = GET_ASSET_PTR_MEMBER(Asset, glyph_info);
            asset_glyph* Src = &Header->Glyph;
        }break;
        
        case AssetType_Array:{
            array_info* Result = GET_ASSET_PTR_MEMBER(Asset, array_info);
            asset_array* Src = &Header->Array;
        }break;
        
        case AssetType_Mesh:{
            mesh_info* Result = GET_ASSET_PTR_MEMBER(Asset, mesh_info);
            asset_mesh* Src = &Header->Mesh;
            
            // NOTE(Dima): Load mesh data
            u32 VertSize = Src->DataVerticesSize;
            u32 IndiSize = Src->DataIndicesSize;
            
            void* Vertices = (u8*)Data + Src->DataOffsetToVertices;
            u32* Indices = (u32*)((u8*)Data + Src->DataOffsetToIndices);
            
            Result->Vertices = Vertices;
            Result->Indices = Indices;
        }break;
        
        case AssetType_Sound:{
            sound_info* Result = GET_ASSET_PTR_MEMBER(Asset, sound_info);
            asset_sound* Src = &Header->Sound;
            
            Result->Samples[0] = (i16*)((u8*)Data + Src->DataOffsetToLeftChannel);
            Result->Samples[1] = (i16*)((u8*)Data + Src->DataOffsetToRightChannel);
        }break;
        
        case AssetType_Font:{
            font_info* Result = GET_ASSET_PTR_MEMBER(Asset, font_info);
            asset_font* Src = &Header->Font;
            
            int* Mapping = (int*)((u8*)Data + Src->DataOffsetToMapping);
            float* KerningPairs = (float*)((u8*)Data + Src->DataOffsetToKerning);
            u32* GlyphIDs = (u32*)((u8*)Data + Src->DataOffsetToIDs);
            
            u32 MappingSize = Src->MappingSize;
            u32 KerningSize = Src->KerningSize;
            u32 IDsSize = Src->IDsSize;
            
            ASSERT(MappingSize == sizeof(float) * FONT_INFO_MAX_GLYPH_COUNT);
            
            // NOTE(Dima): Copy glyph IDs
            Result->GlyphIDs = GlyphIDs;
            
            // NOTE(Dima): Fixing Glyph IDs
            for(int GlyphIndex = 0;
                GlyphIndex < Src->GlyphCount;
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
        
        case AssetType_Model:{
            model_info* Result = GET_ASSET_PTR_MEMBER(Asset, model_info);
            asset_model* Src = &Header->Model;
            
            // NOTE(Dima): Loading and storing model data
            Result->MeshIDs = 0;
            if(Src->MeshCount){
                u32* MeshIDs = (u32*)((u8*)Data + Src->DataOffsetToMeshIDs);
                Result->MeshIDs = MeshIDs;
                
                IntegrateIDs(Result->MeshIDs, Src->MeshCount, FileSource);
            }
            
            
            Result->MaterialIDs = 0;
            if(Src->MaterialCount){
                u32* MaterialIDs = (u32*)((u8*)Data + Src->DataOffsetToMaterialIDs);
                Result->MaterialIDs = MaterialIDs;
                
                IntegrateIDs(Result->MaterialIDs, Src->MaterialCount, FileSource);
            }
            
            Result->SkeletonIDs = 0;
            if(Src->SkeletonCount){
                u32* SkeletonIDs = (u32*)((u8*)Data + Src->DataOffsetToSkeletonIDs);
                Result->SkeletonIDs = SkeletonIDs;
                
                IntegrateIDs(Result->SkeletonIDs, Src->SkeletonCount, FileSource);
            }
            
            Result->AnimationIDs = 0;
            if(Src->AnimationCount){
                u32* AnimationIDs = (u32*)((u8*)Data + Src->DataOffsetToAnimationIDs);
                Result->AnimationIDs = AnimationIDs;
                
                IntegrateIDs(Result->AnimationIDs, Src->AnimationCount, FileSource);
            }
            
            Result->NodesSharedDatas = 0;
            if(Src->NodeCount){
                node_shared_data* NodesSharedDatas = (node_shared_data*)
                    ((u8*)Data + Src->DataOffsetToNodesSharedDatas);
                
                Result->NodesSharedDatas = NodesSharedDatas;
            }
            
            Result->NodeMeshIDsStorage = 0;
            if(Src->NodesMeshIndicesStorageCount){
                u32* MeshIndicesStorage = (u32*)((u8*)Data + Src->DataOffsetToNodesMeshIndicesStorage);
                Result->NodeMeshIDsStorage = MeshIndicesStorage;
                
                IntegrateIDs(Result->NodeMeshIDsStorage, 
                             Src->NodesMeshIndicesStorageCount,
                             FileSource);
            }
            
            // NOTE(Dima): Final nodes setup
            Result->Nodes = (node_info*)((u8*)Data + DataSize);
            for(int NodeIndex = 0;
                NodeIndex < Src->NodeCount;
                NodeIndex++)
            {
                node_info* Node = &Result->Nodes[NodeIndex];
                
                Node->Shared = Result->NodesSharedDatas + NodeIndex;
                Node->MeshIDs = &Result->NodeMeshIDsStorage[Node->Shared->NodeMeshIndexFirstInStorage];
                Node->MeshCount = Node->Shared->NodeMeshIndexCountInStorage;
            }
        }break;
        
        case AssetType_NodeAnimation:{
            node_animation* NodeAnim = GET_ASSET_PTR_MEMBER(Asset, node_animation);
            asset_node_animation* Src = &Header->NodeAnim;
            
            animation_vector_key* PositionKeys = 0;
            if(Src->PositionKeysCount){
                PositionKeys = (animation_vector_key*)((u8*)Data + Src->DataOffsetToPositionKeys);
                NodeAnim->PositionKeys = PositionKeys;
            }
            
            animation_quaternion_key* RotationKeys = 0;
            if(Src->RotationKeysCount){
                RotationKeys = (animation_quaternion_key*)((u8*)Data + Src->DataOffsetToRotataionKeys);
                NodeAnim->RotationKeys = RotationKeys;
            }
            
            animation_vector_key* ScalingKeys = 0;
            if(Src->ScalingKeysCount){
                ScalingKeys = (animation_vector_key*)((u8*)Data + Src->DataOffsetToScalingKeys);
                NodeAnim->ScalingKeys = ScalingKeys;
            }
        }break;
        
        case AssetType_AnimationClip:{
            animation_clip* Clip = GET_ASSET_PTR_MEMBER(Asset, animation_clip);
            asset_animation_clip* Src = &Header->AnimationClip;
            
            u32* NodeAnimationIDs = 0;
            if(Src->NodeAnimationIDsCount){
                NodeAnimationIDs = (u32*)((u8*)Data + Src->DataOffsetToNodeAnimationIDs);
                Clip->NodeAnimationIDs = NodeAnimationIDs;
                
                IntegrateIDs(Clip->NodeAnimationIDs, Src->NodeAnimationIDsCount, FileSource);
            }
            
            if(Src->SizeName){
                Clip->Name = (char*)((u8*)Data + Src->DataOffsetToName);
            }
        }break;
        
        case AssetType_Skeleton:{
            skeleton_info* Result = GET_ASSET_PTR_MEMBER(Asset, skeleton_info);
            asset_skeleton* Src = &Header->Skeleton;
            
            Result->Bones = 0;
            if(Src->BoneCount){
                bone_info* Bones = (bone_info*)((u8*)Data + Src->DataOffsetToBones);
                Result->Bones = Bones;
            }
        }break;
    }
    
    // NOTE(Dima): Setting asset state
    std::atomic_thread_fence(std::memory_order_release);
    Asset->State.store(AssetState_Loaded);
}

struct import_asset_callback_data{
    assets* Assets;
    asset* Asset;
    void* LoadDest;
    u64 LoadDestSize;
    task_data* Task;
};

PLATFORM_CALLBACK(ImportAssetCallback){
    import_asset_callback_data* CallbackData = (import_asset_callback_data*)Data;
    
    void* DestData = CallbackData->LoadDest;
    u64 DataSize = CallbackData->LoadDestSize;
    asset* Asset = CallbackData->Asset;
    assets* Assets = CallbackData->Assets;
    
    ImportAssetDirectly(Assets, Asset, DestData, DataSize);
    
    EndTaskData(&Assets->ImportTasksPool, CallbackData->Task);
}

void ImportAsset(assets* Assets, asset* Asset, b32 Immediate){
    asset_header* Header = &Asset->Header;
    asset_file_source* FileSource = Asset->FileSource;
    
    std::uint32_t ExpectedPrevState = AssetState_Unloaded;
    if(Asset->State.compare_exchange_weak(ExpectedPrevState, 
                                          AssetState_InProgress))
    {
        // NOTE(Dima): Loading data
        u32 DataSize = Header->TotalDataSize;
        u32 DataSizeToAlloc = DataSize;
        
        // NOTE(Dima): Preallocating data for nodes
        if(Asset->Type == AssetType_Model){
            DataSizeToAlloc += sizeof(node_info) * Header->Model.NodeCount;
        }
        
        if(Immediate){
            // TODO(Dima): Change this
            void* Data = malloc(DataSizeToAlloc);
            
            ImportAssetDirectly(Assets, Asset, Data, DataSize);
        }
        else{
            task_data* Task = BeginTaskData(&Assets->ImportTasksPool);
            
            if(Task){
                import_asset_callback_data* CallbackData =
                    (import_asset_callback_data*)Task->Block.Base;
                
                // NOTE(Dima): Loading data
                u32 DataSize = Header->TotalDataSize;
                // TODO(Dima): Change this
                // TODO(Dima): For small sizes use layered allocator
                void* Data = malloc(DataSizeToAlloc);
                
                CallbackData->Assets = Assets;
                CallbackData->Asset = Asset;
                CallbackData->LoadDest = Data;
                CallbackData->LoadDestSize = DataSize;
                CallbackData->Task = Task;
                
                Platform.AddEntry(&Platform.lowPriorityQueue,
                                  ImportAssetCallback, CallbackData);
            }
            else{
                /*NOTE(Dima): If we can not get free memory slot 
                to launch a load asset thread with it - we skip 
                asset loading and assume that next time we will 
                get it.
                */
            }
        }
    }
    else{
        /*NOTE(Dima): If Asset Load State was other value
        we skip loading
        */
    }//State check
}


bmp_info* LoadBmp(assets* Assets,
                  u32 BmpID,
                  b32 Immediate)
{
    bmp_info* Result = LOAD_ASSET(bmp_info, AssetType_Bitmap,
                                  Assets, BmpID, Immediate);
    
    return(Result);
}

font_info* LoadFont(assets* Assets,
                    u32 FontID,
                    b32 Immediate)
{
    font_info* Result = LOAD_ASSET(font_info, AssetType_Font,
                                   Assets, FontID, Immediate);
    
    return(Result);
}


mesh_info* LoadMesh(assets* Assets,
                    u32 MeshID,
                    b32 Immediate)
{
    mesh_info* Result = LOAD_ASSET(mesh_info, AssetType_Mesh,
                                   Assets, MeshID, Immediate);
    
    return(Result);
}

model_info* LoadModel(assets* Assets,
                      u32 ModelID,
                      b32 Immediate)
{
    model_info* Result = LOAD_ASSET(model_info, AssetType_Model,
                                    Assets, ModelID, Immediate);
    
    return(Result);
}


animation_clip* LoadAnimationClip(assets* Assets,
                                  u32 AnimID,
                                  b32 Immediate)
{
    animation_clip* Result = LOAD_ASSET(animation_clip, AssetType_AnimationClip,
                                        Assets, AnimID, Immediate);
    
    return(Result);
}

node_animation* LoadNodeAnim(assets* Assets,
                             u32 NodeAnimID,
                             b32 Immediate)
{
    node_animation* Result = LOAD_ASSET(node_animation, AssetType_NodeAnimation,
                                        Assets, NodeAnimID, Immediate);
    
    return(Result);
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
    
    // NOTE(Dima): Init random generation
    InitRandomGeneration(&Assets->Random, 123);
    
    // NOTE(Dima): Large atlas initialization
    Assets->MainLargeAtlas = InitAtlas(Assets->Memory, 1024);
    
    // NOTE(Dima): Init asset layered memory to allocate asset types
    u32 LayersSizes[] = {64, 128, 256, 512, 1024};
    u32 LayersSizesCount = ARRAY_COUNT(LayersSizes);
    InitLayeredMem(&Assets->LayeredMemory, 
                   Assets->Memory, 
                   LayersSizes, 
                   LayersSizesCount);
    
    // NOTE(Dima): Init tasks datas pool
    InitTaskDataPool(&Assets->ImportTasksPool, Assets->Memory,
                     1024,
                     sizeof(import_asset_callback_data));
    
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
    Platform.OpenFilesBegin("../Data/", "*ja");
    
    platform_file_desc FileDesc;
    
    while(Platform.OpenNextFile(&FileDesc)){
        // NOTE(Dima): Allocating and setting file source
        asset_file_source* FileSource = AllocateFileSource(Assets);
        FileSource->FileDescription = FileDesc;
        FileSource->IntegrationBaseID = 0;
        
        char* FileFullPath = FileSource->FileDescription.FullPath;
        
        asset_file_header FileHeader;
        b32 ReadFileResult = Platform.FileOffsetRead(FileFullPath, 
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
        
        // NOTE(Dima): Reading groups
        asset_file_group *FileGroups = PushArray(&AssetInitMem,
                                                 asset_file_group,
                                                 GameAsset_Count);
        
        Assert(FileHeader.GroupsByteOffset == sizeof(asset_file_header));
        u32 GroupsByteSize = FileHeader.GroupsCount * sizeof(asset_file_group);
        b32 ReadGroupsResult = Platform.FileOffsetRead(FileFullPath,
                                                       FileHeader.GroupsByteOffset,
                                                       GroupsByteSize,
                                                       FileGroups);
        Assert(ReadGroupsResult);
        
        // NOTE(Dima): Reading groups regions
        asset_file_group_region* Regions = PushArray(&AssetInitMem,
                                                     asset_file_group_region,
                                                     FileHeader.RegionsCount);
        
        Assert(FileHeader.GroupsRegionsByteOffset == sizeof(asset_file_header) + GroupsByteSize);
        u32 RegionsByteSize = FileHeader.RegionsCount * sizeof(asset_file_group_region);
        b32 ReadRegionsResult = Platform.FileOffsetRead(FileFullPath,
                                                        FileHeader.GroupsRegionsByteOffset,
                                                        RegionsByteSize,
                                                        Regions);
        Assert(ReadRegionsResult);
        
        // NOTE(Dima): Reading lines offsets
        u32 FileAssetCount = FileHeader.EffectiveAssetsCount;
        u32* AssetLinesOffsets = 0;
        
        if(FileAssetCount){
            AssetLinesOffsets = PushArray(&AssetInitMem, u32, FileAssetCount);
            b32 ReadOffsetsRes = Platform.FileOffsetRead(FileFullPath,
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
                RegionIndex < FileGrp->RegionCount;
                RegionIndex++)
            {
                // NOTE(Dima): Getting right region from big file regions array
                asset_file_group_region* Reg = Regions + FileGrp->FirstRegionIndex + RegionIndex;
                
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
                    
                    b32 ReadAssetHeader = Platform.FileOffsetRead(
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
                    NewAsset->TagCount = AssetHeader.TagCount;
                    
                    // NOTE(Dima): Reading tags
                    if(AssetHeader.TagCount){
                        u32 TagsSizeToRead = AssetHeader.TagCount * sizeof(asset_tag_header);
                        if(TagsSizeToRead > sizeof(NewAsset->Tags)){
                            TagsSizeToRead = sizeof(NewAsset->Tags);
                            NewAsset->TagCount = MAX_TAGS_PER_ASSET;
                        }
                        
                        b32 ReadTagsResult = Platform.FileOffsetRead(
                            FileFullPath,
                            LineOffset + AssetHeader.LineTagOffset,
                            TagsSizeToRead,
                            NewAsset->Tags);
                    }
                    
                    DLIST_INSERT_BEFORE_SENTINEL(NewAsset, ToGroup->Sentinel, Next, Prev);
                    ToGroup->InGroupAssetCount++;
                    
                    u32 DataOffsetInFile = LineOffset + AssetHeader.LineDataOffset;
                    NewAsset->OffsetToData = DataOffsetInFile;
                    
#define ALLOC_ASS_PTR_MEMBER(type) (type*)AllocateAssetType(Assets, NewAsset, (void**)&GET_ASSET_PTR_MEMBER(NewAsset, type), sizeof(type))
                    
                    
                    // NOTE(Dima): Initializing assets
                    // NOTE(Dima): Loading description info from file headers
                    switch(NewAsset->Type){
                        case AssetType_Bitmap: {
                            bmp_info* Result = ALLOC_ASS_PTR_MEMBER(bmp_info);
                            asset_bitmap* Src = &AssetHeader.Bitmap;
                            
                            if(Src->BakeToAtlas){
                                ImportAsset(Assets, NewAsset, ASSET_IMPORT_IMMEDIATE);
                                bmp_info* Bmp = GET_ASSET_PTR_MEMBER(NewAsset, bmp_info);
                                AddBitmapToAtlas(&Assets->MainLargeAtlas, Bmp);
                            }
                            else{
                                // NOTE(Dima): Initializing bitmap & set pixels to 0
                                AllocateBitmapInternal(Result, Src->Width, Src->Height, 0);
                            }
                        }break;
                        
                        case AssetType_Array: {
                            array_info* Result = ALLOC_ASS_PTR_MEMBER(array_info);
                            asset_array* Src = &AssetHeader.Array;
                            
                            Result->FirstID = Src->FirstID;
                            Result->Count = Src->Count;
                            
                        }break;
                        
                        case AssetType_Mesh: {
                            mesh_info* Result = ALLOC_ASS_PTR_MEMBER(mesh_info);
                            asset_mesh* Src = &AssetHeader.Mesh;
                            
                            Result->VerticesCount = Src->VerticesCount;
                            Result->IndicesCount = Src->IndicesCount;
                            Result->TypeCtx = Src->TypeCtx;
                            
                            // NOTE(Dima): Checking correctness of loaded vertices type sizes
                            switch(Result->TypeCtx.MeshType){
                                case Mesh_Simple:{
                                    ASSERT(Src->TypeCtx.VertexTypeSize == sizeof(vertex_info));
                                }break;
                                
                                case Mesh_Skinned:{
                                    ASSERT(Src->TypeCtx.VertexTypeSize == sizeof(vertex_skinned_info));
                                }break;
                            }
                            
                        }break;
                        
                        case AssetType_Sound: {
                            sound_info* Result = ALLOC_ASS_PTR_MEMBER(sound_info);
                            asset_sound* Src = &AssetHeader.Sound;
                            
                            Result->SampleCount = Src->SampleCount;
                            Result->SamplesPerSec = Src->SamplesPerSec;
                            Result->Channels = Src->Channels;
                            
                        }break;
                        
                        case AssetType_Font: {
                            font_info* Result = ALLOC_ASS_PTR_MEMBER(font_info);
                            asset_font* Src = &AssetHeader.Font;
                            
                            Result->AscenderHeight = Src->AscenderHeight;
                            Result->DescenderHeight = Src->DescenderHeight;
                            Result->LineGap = Src->LineGap;
                            Result->GlyphCount = Src->GlyphCount;
                            
                        }break;
                        
                        case AssetType_Glyph: {
                            glyph_info* Result = ALLOC_ASS_PTR_MEMBER(glyph_info);
                            asset_glyph* Src = &AssetHeader.Glyph;
                            
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
                        
                        case AssetType_Material:{
                            
                        }break;
                        
                        case AssetType_Model:{
                            model_info* Result = ALLOC_ASS_PTR_MEMBER(model_info);
                            asset_model* Src = &AssetHeader.Model;
                            
                            Result->MeshCount = Src->MeshCount;
                            Result->MaterialCount = Src->MaterialCount;
                            Result->SkeletonCount = Src->SkeletonCount;
                            Result->NodeCount = Src->NodeCount;
                            Result->NodesMeshIDsStorageCount = Src->NodesMeshIndicesStorageCount;
                            Result->AnimationCount = Src->AnimationCount;
                        }break;
                        
                        case AssetType_NodeAnimation:{
                            node_animation* NodeAnim = ALLOC_ASS_PTR_MEMBER(node_animation);
                            asset_node_animation* Src = &AssetHeader.NodeAnim;
                            
                            NodeAnim->PositionKeysCount = Src->PositionKeysCount;
                            NodeAnim->RotationKeysCount = Src->RotationKeysCount;
                            NodeAnim->ScalingKeysCount = Src->ScalingKeysCount;
                            NodeAnim->NodeIndex = Src->NodeIndex;
                        }break;
                        
                        case AssetType_AnimationClip:{
                            animation_clip* Clip = ALLOC_ASS_PTR_MEMBER(animation_clip);
                            asset_animation_clip* Src = &AssetHeader.AnimationClip;
                            
                            Clip->IsLooping = true;
                            Clip->DurationTicks = Src->Duration;
                            Clip->TicksPerSecond = Src->TicksPerSecond;
                            Clip->NodeAnimationsCount = Src->NodeAnimationIDsCount;
                        }break;
                        
                        case AssetType_Skeleton:{
                            skeleton_info* Result = ALLOC_ASS_PTR_MEMBER(skeleton_info);
                            asset_skeleton* Src = &AssetHeader.Skeleton;
                            
                            Result->BoneCount = Src->BoneCount;
                            
                            // TODO(Dima): Use checksum later to load only not loaded skeletons
                        }break;
                    }
                    
                    // NOTE(Dima): If we should load asset immediately - do it
                    if(AssetHeader.ImmediateLoad){
                        ImportAsset(Assets, NewAsset, ASSET_IMPORT_IMMEDIATE);
                    }
                }
            }
        }
        
        FreeNoDealloc(&AssetInitMem);
    }
    
    Platform.OpenFilesEnd();
    
    Free(&AssetInitMem);
    
    // NOTE(Dima): Setting assets arrays
    for(int GroupIndex = 0; 
        GroupIndex < GameAsset_Count;
        GroupIndex++)
    {
        asset_group* Group = &Assets->Groups[GroupIndex];
        
        Group->PointersToAssets = PushArray(Assets->Memory, 
                                            asset*, 
                                            Group->InGroupAssetCount);
        
        asset* At = Group->Sentinel.Next;
        for(int Index = 0; Index < Group->InGroupAssetCount; Index++){
            Group->PointersToAssets[Index] = At;
            
            At = At->Next;
        }
    }
}