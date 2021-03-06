INTERNAL_FUNCTION asset_slot* AllocateAssetSlot(asset_system* Assets)
{
    DLIST_ALLOCATE_FUNCTION_BODY(asset_slot,
                                 Assets->Memory,
                                 NextAlloc, PrevAlloc,
                                 Assets->FreeSlot,
                                 Assets->UseSlot,
                                 1024, Result);
    
    return(Result);
}


INTERNAL_FUNCTION void AddBitmapToAtlas(Asset_Atlas* atlas, 
                                        bmp_info* bmp)
{
    In_Atlas_Bitmap Result = {};
    
    int Border = 3;
    
    int ActualWidth = bmp->Prim.Width + Border * 2;
    int ActualHeight = bmp->Prim.Height + Border * 2;
    
    if(atlas->AtX + ActualWidth > atlas->Dim){
        atlas->AtX = 0;
        atlas->AtY = atlas->AtY + atlas->MaxInRowHeight;
        atlas->MaxInRowHeight = 0;
    }
    
    ASSERT(atlas->AtY + ActualHeight  < atlas->Dim);
    
    float OneOverDim = atlas->OneOverDim;
    
    for (int YIndex = 0; YIndex < bmp->Prim.Height; YIndex++) {
        
        for (int XIndex = 0; XIndex < bmp->Prim.Width; XIndex++) {
            u32* At = (u32*)bmp->Prim.Data + YIndex * bmp->Prim.Width + XIndex;
            u32* To = (u32*)atlas->Bitmap.Prim.Data + (atlas->AtY + YIndex + Border) * atlas->Dim + atlas->AtX + XIndex + Border;
            
            *To = *At;
        }
    }
    
    bmp->Prim.MinUV = V2(
                         (float)(atlas->AtX + Border) * OneOverDim, 
                         (float)(atlas->AtY + Border) * OneOverDim);
    
    bmp->Prim.MaxUV = V2(
                         (float)(atlas->AtX + Border + bmp->Prim.Width) * OneOverDim,
                         (float)(atlas->AtY + Border + bmp->Prim.Height) * OneOverDim);
    
    atlas->AtX += ActualWidth;
    atlas->MaxInRowHeight = Max(atlas->MaxInRowHeight, ActualHeight);
}

INTERNAL_FUNCTION Asset_Atlas InitAtlas(mem_arena* Region, int Dim){
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

u32 GetFirst(asset_system* Assets, u32 EntryIndex){
    asset_entry* Entry = &Assets->Entries[EntryIndex];
    
    u32 Result = 0;
    
    if(Entry->InEntryAssetCount){
        Result = Entry->PointersToAssets[0]->ID;
    }
    
    return(Result);
}

u32 GetRandom(asset_system* Assets, u32 EntryIndex){
    asset_entry* Entry = &Assets->Entries[EntryIndex];
    
    u32 Result = 0;
    
    if(Entry->InEntryAssetCount){
        int RandomIndex = GetRandomIndex(&Assets->Random, 
                                         Entry->InEntryAssetCount);
        
        Result = Entry->PointersToAssets[RandomIndex]->ID;
    }
    
    return(Result);
}

u32 GetBestByTags(asset_system* Assets, 
                  u32 Group, 
                  u32* TagTypes, 
                  asset_tag_value* TagValues, 
                  int TagsCount)
{
    asset_entry* Entry = &Assets->Entries[Group];
    
    u32 Result = 0;
    
    /*
TODO(Dima): Ideas to make it faster:
*/
    
#if 1
    if(Entry->InEntryAssetCount){
        int BestIndex = 0;
        float BestWeight = 0.0f;
        
        for(int EntryAssetIndex = 0; 
            EntryAssetIndex < Entry->InEntryAssetCount;
            EntryAssetIndex++)
        {
            asset* Asset = Entry->PointersToAssets[EntryAssetIndex];
            
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
                        switch(Tag->Value.Type){
                            case TagValue_Float:{
                                float Diff = abs(Tag->Value.Value_Float - Value->Value_Float);
                                
                                Weight += (1.0f - Diff);
                            }break;
                            
                            case TagValue_Int:{
                                int Diff = Abs(Tag->Value.Value_Int - Value->Value_Int);
                                
                                Weight += 1.0f - ((float)(Diff) * 0.01f);
                            }break;
                            
                            case TagValue_Empty:{
                                BestIndex = EntryAssetIndex;
                                
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
                BestIndex = EntryAssetIndex;
            }
            
            if(ShouldExitAssetLoop){
                break;
            }
        }
        
        Result = Entry->PointersToAssets[BestIndex]->ID;
    }
#endif
    
#if 0    
    int BestIndex = 0;
    float BestWeight = 0.0f;
    
    for(int MatchTagIndex = 0;
        MatchTagIndex < TagsCount;
        MatchTagIndex++)
    {
        asset_tag_value* Value = &TagValues[MatchTagIndex];
        u32 TagType = TagTypes[MatchTagIndex];
        
        asset_entry* Group = &Assets->TagGroups[TagType][Group];
        
        for(int InGroupIndex = 0;
            InGroupIndex < Group->InGroupAssetCount;
            InGroupIndex++)
        {
            asset* Asset = Group->PointersToAssets[InGroupIndex];
            
            // NOTE(Dima): We found needed tag
            switch(Tag->Value.Type){
                case TagValue_Float:{
                    float Diff = abs(Tag->Value.Value_Float - Value->Value_Float);
                    
                    Weight += (1.0f - Diff);
                }break;
                
                case TagValue_Int:{
                    int Diff = Abs(Tag->Value.Value_Int - Value->Value_Int);
                    
                    Weight += 1.0f - ((float)(Diff) * 0.01f);
                }break;
            }
            
            if(Weight > BestWeight){
                BestWeight = Weight;
                BestIndex = EntryAssetIndex;
            }
            
        }
    }
    
    Result = Entry->PointersToAssets[BestIndex]->ID;
#endif
    
    return(Result);
}

inline asset_file_source* AllocateFileSource(asset_system* Assets){
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

void* AllocateAssetType(asset_system* Assets, asset* Asset, void** Type, u32 AssetTypeSize){
    Asset->TypeMemEntry = AllocateMemLayerEntry(
                                                &Assets->LayeredMemory, AssetTypeSize);
    
    ASSERT(Asset->TypeMemEntry);
    
    void* Result = Asset->TypeMemEntry->Data;
    
    Platform.MemZeroRaw(Result, AssetTypeSize);
    
    *Type = Result;
    
    return(Result);
}

INTERNAL_FUNCTION void AddAssetToEntry(asset_system* Assets, 
                                       asset_entry* Entry, 
                                       asset* ToAdd)
{
    asset_slot* Slot = AllocateAssetSlot(Assets);
    
    Slot->Asset = ToAdd;
    
    DLIST_INSERT_BEFORE_SENTINEL(Slot, Entry->Sentinel, Next, Prev);
    Entry->InEntryAssetCount++;
}

INTERNAL_FUNCTION void InitAssetEntries(asset_entry* Entries, int Count){
    for(int EntryIndex = 0; 
        EntryIndex < AssetEntry_Count;
        EntryIndex++)
    {
        asset_entry* Entry = &Entries[EntryIndex];
        
        Entry->PointersToAssets = 0;
        Entry->InEntryAssetCount = 0;
        
        DLIST_REFLECT_PTRS(Entry->Sentinel, Next, Prev);
    }
}

INTERNAL_FUNCTION void EntriesFillAssetArray(asset_system* Assets, asset_entry* Entries, int Count){
    for(int EntryIndex = 0; 
        EntryIndex < Count;
        EntryIndex++)
    {
        asset_entry* Entry = &Entries[EntryIndex];
        
        if(Entry->InEntryAssetCount){
            
            Entry->PointersToAssets = PushArray(Assets->Memory, 
                                                asset*,
                                                Entry->InEntryAssetCount);
            
            asset_slot* At = Entry->Sentinel.Next;
            for(int Index = 0; Index < Entry->InEntryAssetCount; Index++){
                Entry->PointersToAssets[Index] = At->Asset;
                
                At = At->Next;
            }
        }
        else{
            Entry->PointersToAssets = 0;
        }
    }
}

INTERNAL_FUNCTION void AddAssetToCorrespondingEntries(asset_system* Assets, asset* ToAdd)
{
    AddAssetToEntry(Assets, &Assets->Entries[ToAdd->EntryIndex], ToAdd);
    
    for(int TagTypeIndex = 0; TagTypeIndex < AssetTag_Count; TagTypeIndex++){
        for(int TagIndex = 0; TagIndex < ToAdd->TagCount; TagIndex++){
            asset_tag_header* TagHeader = &ToAdd->Tags[TagIndex];
            
            if(TagHeader->Type == TagTypeIndex){
                asset_entry* EntryArray = Assets->TaggedEntries[TagTypeIndex];
                AddAssetToEntry(Assets, &EntryArray[ToAdd->EntryIndex], ToAdd);
            }
        }
    }
}

INTERNAL_FUNCTION void FillCorrespondingAssetArrays(asset_system* Assets){
    EntriesFillAssetArray(Assets, Assets->Entries, AssetEntry_Count);
    
    for(int TagIndex = 0; TagIndex < AssetTag_Count; TagIndex++){
        EntriesFillAssetArray(Assets, 
                              Assets->TaggedEntries[TagIndex], 
                              AssetEntry_Count);
    }
}

INTERNAL_FUNCTION void InitCorrespondingEntries(asset_system* Assets){
    InitAssetEntries(Assets->Entries, AssetEntry_Count);
    
    for(int TagIndex = 0; TagIndex < AssetTag_Count; TagIndex++){
        InitAssetEntries(Assets->TaggedEntries[TagIndex], 
                         AssetEntry_Count);
    }
}

#define GET_DATA(type, offset) (type*)((u8*)Data + (offset))

void ImportAssetDirectly(asset_system* Assets, 
                         asset* Asset, 
                         void* Data, 
                         u64 DataSize,
                         mem_arena* TempStorageBlock)
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
            
            
            void* Vertices = GET_DATA(void, Src->DataOffsetToVertices);
            u32* Indices = GET_DATA(u32, Src->DataOffsetToIndices);
            
            Result->Prim.Vertices = Vertices;
            Result->Prim.Indices = Indices;
        }break;
        
        case AssetType_Sound:{
            sound_info* Result = GET_ASSET_PTR_MEMBER(Asset, sound_info);
            asset_sound* Src = &Header->Sound;
            
            Result->Samples[0] = GET_DATA(i16, Src->DataOffsetToLeftChannel);
            Result->Samples[1] = GET_DATA(i16, Src->DataOffsetToRightChannel);
        }break;
        
        case AssetType_Font:{
            font_info* Result = GET_ASSET_PTR_MEMBER(Asset, font_info);
            asset_font* Src = &Header->Font;
            
            int* Mapping = GET_DATA(int, Src->DataOffsetToMapping);
            float* KerningPairs = GET_DATA(float, Src->DataOffsetToKerning);
            u32* GlyphIDs = GET_DATA(u32, Src->DataOffsetToIDs);
            
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
        
        case AssetType_Model:
        {
            model_info* Result = GET_ASSET_PTR_MEMBER(Asset, model_info);
            asset_model* Src = &Header->Model;
            
            // NOTE(Dima): Loading and storing model data
            Result->MeshIDs = 0;
            if(Src->MeshCount){
                u32* MeshIDs = GET_DATA(u32, Src->DataOffsetToMeshIDs);
                Result->MeshIDs = MeshIDs;
                
                IntegrateIDs(Result->MeshIDs, Src->MeshCount, FileSource);
            }
            
            Result->MaterialIDs = 0;
            if(Src->MaterialCount){
                u32* MaterialIDs = GET_DATA(u32, Src->DataOffsetToMaterialIDs);
                Result->MaterialIDs = MaterialIDs;
                
                IntegrateIDs(Result->MaterialIDs, Src->MaterialCount, FileSource);
            }
            
            Result->AnimationIDs = 0;
            if(Src->AnimationCount){
                u32* AnimationIDs = GET_DATA(u32, Src->DataOffsetToAnimationIDs);
                Result->AnimationIDs = AnimationIDs;
                
                IntegrateIDs(Result->AnimationIDs, Src->AnimationCount, FileSource);
            }
            
            Result->NodesSharedDatas = 0;
            if(Src->NodeCount){
                node_shared_data* NodesSharedDatas = GET_DATA(node_shared_data, Src->DataOffsetToNodesSharedDatas);
                
                Result->NodesSharedDatas = NodesSharedDatas;
            }
            
            Result->NodeMeshIDsStorage = 0;
            if(Src->NodesMeshIndicesStorageCount){
                u32* MeshIndicesStorage = GET_DATA(u32, Src->DataOffsetToNodesMeshIndicesStorage);
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
                
                Node->Shared = &Result->NodesSharedDatas[NodeIndex];
                Node->MeshIDs = &Result->NodeMeshIDsStorage[Node->Shared->NodeMeshIndexFirstInStorage];
                Node->MeshCount = Node->Shared->NodeMeshIndexCountInStorage;
            }
        }break;
        
        case AssetType_NodeAnimation:
        {
            node_animation* NodeAnim = GET_ASSET_PTR_MEMBER(Asset, node_animation);
            asset_node_animation* Src = &Header->NodeAnim;
            
            NodeAnim->PositionKeysValues = 0;
            NodeAnim->PositionKeysTimes = 0;
            NodeAnim->BeginP = V3_Zero();
            NodeAnim->EndP = V3_Zero();
            if(Src->PositionKeysCount){
                v3* PositionKeysValues = GET_DATA(v3, Src->DataOffsetToPositionKeysValues);
                float* PositionKeysTimes = GET_DATA(float, Src->DataOffsetToPositionKeysTimes);
                
                NodeAnim->PositionKeysValues = PositionKeysValues;
                NodeAnim->PositionKeysTimes = PositionKeysTimes;
                
                NodeAnim->BeginP = PositionKeysValues[0];
                NodeAnim->EndP = PositionKeysValues[Src->PositionKeysCount - 1];
            }
            
            
            NodeAnim->RotationKeysValues = 0;
            NodeAnim->RotationKeysTimes = 0;
            NodeAnim->BeginR = IdentityQuaternion();
            NodeAnim->EndR = IdentityQuaternion();
            if(Src->RotationKeysCount){
                quat* RotationKeysValues = GET_DATA(quat, Src->DataOffsetToRotationKeysValues);
                float* RotationKeysTimes = GET_DATA(float, Src->DataOffsetToRotationKeysTimes);
                
                NodeAnim->RotationKeysValues = RotationKeysValues;
                NodeAnim->RotationKeysTimes = RotationKeysTimes;
                
                NodeAnim->BeginR = RotationKeysValues[0];
                NodeAnim->EndR = RotationKeysValues[Src->RotationKeysCount - 1];
            }
            
            NodeAnim->ScalingKeysValues = 0;
            NodeAnim->ScalingKeysTimes = 0;
            NodeAnim->BeginS = V3_One();
            NodeAnim->EndS = V3_One();
            if(Src->ScalingKeysCount){
                v3* ScalingKeysValues = GET_DATA(v3, Src->DataOffsetToScalingKeysValues);
                float* ScalingKeysTimes = GET_DATA(float, Src->DataOffsetToScalingKeysTimes);
                
                NodeAnim->ScalingKeysValues = ScalingKeysValues;
                NodeAnim->ScalingKeysTimes = ScalingKeysTimes;
                
                NodeAnim->BeginS = ScalingKeysValues[0];
                NodeAnim->EndS = ScalingKeysValues[Src->ScalingKeysCount - 1];
            }
            
            // TODO(Dima): Remove this. It's for test
            if(Src->IsRootMotion)
            {
                int a = 1;
            }
            
        }break;
        
        case AssetType_AnimationClip:
        {
            animation_clip* Clip = GET_ASSET_PTR_MEMBER(Asset, animation_clip);
            asset_animation_clip* Src = &Header->AnimationClip;
            
            Clip->NodeAnimationIDs = 0;
            if(Src->NodeAnimationIDsCount){
                u32* NodeAnimationIDs = GET_DATA(u32, Src->DataOffsetToNodeAnimationIDs);
                Clip->NodeAnimationIDs = NodeAnimationIDs;
                
                IntegrateIDs(Clip->NodeAnimationIDs, Src->NodeAnimationIDsCount, FileSource);
            }
            
            if(Src->SizeName){
                Clip->Name = GET_DATA(char, Src->DataOffsetToName);
            }
        }break;
        
        case AssetType_Skeleton:
        {
            skeleton_info* Result = GET_ASSET_PTR_MEMBER(Asset, skeleton_info);
            asset_skeleton* Src = &Header->Skeleton;
            
            Result->Bones = 0;
            if(Src->BoneCount){
                bone_info* Bones = GET_DATA(bone_info, Src->DataOffsetToBones);
                Result->Bones = Bones;
            }
        }break;
    }
    
    // NOTE(Dima): Setting asset state
    std::atomic_thread_fence(std::memory_order_seq_cst);
    Asset->State.store(AssetState_Loaded);
}

struct import_asset_callback_data{
    asset_system* Assets;
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
    asset_system* Assets = CallbackData->Assets;
    
    mem_arena TempArena = CreateInRestOfRegion(&CallbackData->Task->Arena);
    
    ImportAssetDirectly(Assets, Asset, DestData, DataSize, &TempArena);
    
    EndTaskData(&Assets->ImportTasksPool, CallbackData->Task);
}

void ImportAsset(asset_system* Assets, asset* Asset, b32 Immediate){
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
            
            // NOTE(Dima): As this pool is only for main thread - 
            // we should always get the task memory
            task_data* TempTask = BeginTaskData(&Assets->ImportTaskPoolMainThread);
            Assert(TempTask);
            
            ImportAssetDirectly(Assets, Asset, Data, DataSize, &TempTask->Arena);
            
            EndTaskData(&Assets->ImportTaskPoolMainThread, TempTask);
        }
        else{
            task_data* Task = BeginTaskData(&Assets->ImportTasksPool);
            
            if(Task){
                import_asset_callback_data* CallbackData = PushStruct(&Task->Arena, import_asset_callback_data);
                
                // TODO(Dima): Change this
                // TODO(Dima): For small sizes use layered allocator
                void* Data = malloc(DataSizeToAlloc);
                
                CallbackData->Assets = Assets;
                CallbackData->Asset = Asset;
                CallbackData->LoadDest = Data;
                CallbackData->LoadDestSize = DataSize;
                CallbackData->Task = Task;
                
                Platform.AddEntry(Platform.AsyncQueue,
                                  ImportAssetCallback, CallbackData);
            }
            else{
                /*NOTE(Dima): If we can not get free memory slot 
                to launch a load asset thread with it - we skip 
                asset loading and assume that next time we will 
                get it.
                */
                
                Asset->State.store(AssetState_Unloaded);
            }
        }
    }
    else{
        /*NOTE(Dima): If Asset Load State was other value
        we skip loading
        */
    }//State check
}


array_info* LoadArray(asset_system* Assets,
                      u32 ArrayID)
{
    array_info* Result = LOAD_ASSET(array_info, AssetType_Array,
                                    Assets, ArrayID, ASSET_IMPORT_IMMEDIATE);
    
    return(Result);
}

bmp_info* LoadBmp(asset_system* Assets,
                  u32 BmpID,
                  b32 Immediate)
{
    bmp_info* Result = LOAD_ASSET(bmp_info, AssetType_Bitmap,
                                  Assets, BmpID, Immediate);
    
    return(Result);
}

font_info* LoadFont(asset_system* Assets,
                    u32 FontID,
                    b32 Immediate)
{
    font_info* Result = LOAD_ASSET(font_info, AssetType_Font,
                                   Assets, FontID, Immediate);
    
    return(Result);
}


mesh_info* LoadMesh(asset_system* Assets,
                    u32 MeshID,
                    b32 Immediate)
{
    mesh_info* Result = LOAD_ASSET(mesh_info, AssetType_Mesh,
                                   Assets, MeshID, Immediate);
    
    return(Result);
}


material_info* LoadMaterial(asset_system* Assets,
                            u32 MaterialID,
                            b32 Immediate)
{
    material_info* Result = LOAD_ASSET(material_info, AssetType_Material,
                                       Assets, MaterialID, Immediate);
    
    asset* Asset = GetAssetByID(Assets, MaterialID);
    ASSERT(Asset->Type == AssetType_Material);
    
    asset_material* Src = &Asset->Header.Material;
    
    if(Result){
        for(int ChannelIndex = 0;
            ChannelIndex < MaterialChannel_Count;
            ChannelIndex++)
        {
            // NOTE(Dima): I only use first texture index of array
            u32 TextureBmpID = FileToIntegratedID(Asset->FileSource, 
                                                  Src->TextureIDs[ChannelIndex]);
            
            bmp_info* LoadedBitmap = LoadBmp(Assets, TextureBmpID, Immediate);
            Result->Prim.Textures[ChannelIndex] = &LoadedBitmap->Prim;
        }
    }
    
    return(Result);
}

model_info* LoadModel(asset_system* Assets,
                      u32 ModelID,
                      b32 Immediate)
{
    model_info* Result = LOAD_ASSET(model_info, AssetType_Model,
                                    Assets, ModelID, Immediate);
    
    return(Result);
}

skeleton_info* LoadSkeleton(asset_system* Assets,
                            u32 SkeletonID,
                            b32 Immediate)
{
    skeleton_info* Result = LOAD_ASSET(skeleton_info, AssetType_Skeleton,
                                       Assets, SkeletonID, Immediate);
    
    return(Result);
}

animation_clip* LoadAnimationClip(asset_system* Assets,
                                  u32 AnimID,
                                  b32 Immediate)
{
    animation_clip* Result = LOAD_ASSET(animation_clip, AssetType_AnimationClip,
                                        Assets, AnimID, Immediate);
    
    return(Result);
}

node_animation* LoadNodeAnim(asset_system* Assets,
                             u32 NodeAnimID,
                             b32 Immediate)
{
    node_animation* Result = LOAD_ASSET(node_animation, AssetType_NodeAnimation,
                                        Assets, NodeAnimID, Immediate);
    
    return(Result);
}

inline asset* AllocateAsset(asset_system* Assets, asset_file_source* FileSource, u32 FileID)
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

INTERNAL_FUNCTION void InitAssets(asset_system* Assets)
{
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // NOTE(Dima): Memory region is already initialized
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    // NOTE(Dima): Init random generation
    InitRandomGeneration(&Assets->Random, 123);
    
    // NOTE(Dima): Large atlas initialization
    Assets->MainLargeAtlas = InitAtlas(Assets->Memory, 2048);
    
    // NOTE(Dima): Init asset layered memory to allocate asset types
    u32 LayersSizes[] = {64, 128, 256, 512, 1024, 2048, 4096};
    u32 LayersSizesCount = ARRAY_COUNT(LayersSizes);
    InitLayeredMem(&Assets->LayeredMemory, 
                   Assets->Memory, 
                   LayersSizes, 
                   LayersSizesCount);
    
    // NOTE(Dima): Init tasks datas pools
    mi SizeForAssetImport = Kilobytes(256);
    
    InitTaskDataPool(&Assets->ImportTaskPoolMainThread, 
                     Assets->Memory,
                     1,
                     SizeForAssetImport);
    
    InitTaskDataPool(&Assets->ImportTasksPool, 
                     Assets->Memory,
                     4,
                     SizeForAssetImport);
    
    // NOTE(Dima): Init asset files sources
    DLIST_REFLECT_PTRS(Assets->FileSourceUse, Next, Prev);
    DLIST_REFLECT_PTRS(Assets->FileSourceFree, Next, Prev);
    
    // NOTE(Dima): Init slot sentinels
    DLIST_REFLECT_PTRS(Assets->UseSlot, NextAlloc, PrevAlloc);
    DLIST_REFLECT_PTRS(Assets->FreeSlot, NextAlloc, PrevAlloc);
    
    InitCorrespondingEntries(Assets);
    
    mem_arena AssetInitMem = {};
    
    // NOTE(Dima): Init first null asset
    Assets->AssetBlocks[0].BlockAssets = PushArray(Assets->Memory, asset, MAX_ASSETS_IN_ASSET_BLOCK);
    Assets->AssetBlocks[0].InBlockCount = 1;
    
    // NOTE(Dima): Temp initialization of asset families
    for(int FamIndex = 0;
        FamIndex < AssetEntry_Count;
        FamIndex++)
    {
        asset_entry* Entry = &Assets->Entries[FamIndex];
        
        Entry->InEntryAssetCount = 0;
        DLIST_REFLECT_PTRS(Entry->Sentinel, Next, Prev);
    }
    
    // NOTE(Dima): Loading from asset files
    platform_file_desc FileDesc;
    Platform.OpenFilesBegin("../Data/", "*jass");
    
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
        Assert(FileHeader.EntriesCount == AssetEntry_Count);
        
        // NOTE(Dima): Reading groups
        asset_file_group *FileEntries = PushArray(&AssetInitMem,
                                                  asset_file_group,
                                                  AssetEntry_Count);
        
        Assert(FileHeader.EntriesByteOffset == sizeof(asset_file_header));
        u32 EntriesByteSize = FileHeader.EntriesCount * sizeof(asset_file_group);
        b32 ReadEntriesResult = Platform.FileOffsetRead(FileFullPath,
                                                        FileHeader.EntriesByteOffset,
                                                        EntriesByteSize,
                                                        FileEntries);
        Assert(ReadEntriesResult);
        
        // NOTE(Dima): Reading groups regions
        asset_file_group_region* Regions = PushArray(&AssetInitMem,
                                                     asset_file_group_region,
                                                     FileHeader.RegionsCount);
        
        Assert(FileHeader.EntriesRegionsByteOffset == sizeof(asset_file_header) + EntriesByteSize);
        u32 RegionsByteSize = FileHeader.RegionsCount * sizeof(asset_file_group_region);
        b32 ReadRegionsResult = Platform.FileOffsetRead(FileFullPath,
                                                        FileHeader.EntriesRegionsByteOffset,
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
        for(int FileEntryIndex = 0; 
            FileEntryIndex < AssetEntry_Count; 
            FileEntryIndex++)
        {
            asset_file_group* FileEntry = FileEntries + FileEntryIndex;
            asset_entry* ToEntry = Assets->Entries + FileEntryIndex;
            
            // NOTE(Dima): Iterating through regions in file
            for(int RegionIndex = 0;
                RegionIndex < FileEntry->RegionCount;
                RegionIndex++)
            {
                // NOTE(Dima): Getting right region from big file regions array
                asset_file_group_region* Reg = Regions + FileEntry->FirstRegionIndex + RegionIndex;
                
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
                    
                    NewAsset->Data = {};
                    NewAsset->State = AssetState_Unloaded;
                    NewAsset->Header = AssetHeader;
                    NewAsset->FileSource = FileSource;
                    NewAsset->Type = AssetHeader.AssetType;
                    NewAsset->TagCount = AssetHeader.TagCount;
                    NewAsset->EntryIndex = FileEntryIndex;
                    
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
                    
                    /*
// NOTE(Dima):  After tags being written we can add tag to 
corresponding asset groups (Main groups, and tag groups for each tag
*/
                    AddAssetToCorrespondingEntries(Assets, NewAsset);
                    
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
                            
                            Result->FirstID = FileToIntegratedID(FileSource, Src->FirstID);
                            Result->Count = Src->Count;
                            
                        }break;
                        
                        case AssetType_Mesh: {
                            mesh_info* Result = ALLOC_ASS_PTR_MEMBER(mesh_info);
                            asset_mesh* Src = &AssetHeader.Mesh;
                            
                            Result->MaterialIndexInModel = Src->MaterialIndex;
                            Result->Prim.VerticesCount = Src->VerticesCount;
                            Result->Prim.IndicesCount = Src->IndicesCount;
                            Result->Prim.TypeCtx = Src->TypeCtx;
                            
                            // NOTE(Dima): Checking correctness of loaded vertices type sizes
                            switch(Result->Prim.TypeCtx.MeshType){
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
                            material_info* Result = ALLOC_ASS_PTR_MEMBER(material_info);
                            asset_material* Src = &AssetHeader.Material;
                            
                            for(int ChannelIndex = 0;
                                ChannelIndex < MaterialChannel_Count;
                                ChannelIndex++)
                            {
                                Result->Prim.Colors[ChannelIndex] = UnpackRGB_R10G12B10(Src->PackedColors[ChannelIndex]);
                                Result->Prim.Textures[ChannelIndex] = 0;
                            }
                        }break;
                        
                        case AssetType_Model:{
                            model_info* Result = ALLOC_ASS_PTR_MEMBER(model_info);
                            asset_model* Src = &AssetHeader.Model;
                            
                            Result->SkeletonID = FileToIntegratedID(FileSource, Src->SkeletonID);
                            Result->HasSkeleton = Src->SkeletonID != 0;
                            Result->NodesCheckSum = Src->NodesCheckSum;
                            
                            Result->MeshCount = Src->MeshCount;
                            Result->MaterialCount = Src->MaterialCount;
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
                            NodeAnim->IsRootMotion = Src->IsRootMotion;
                        }break;
                        
                        case AssetType_AnimationClip:{
                            animation_clip* Clip = ALLOC_ASS_PTR_MEMBER(animation_clip);
                            asset_animation_clip* Src = &AssetHeader.AnimationClip;
                            
                            Clip->DurationTicks = Src->Duration;
                            Clip->TicksPerSecond = Src->TicksPerSecond;
                            Clip->NodeAnimationsCount = Src->NodeAnimationIDsCount;
                            Clip->NodesCheckSum = Src->NodesCheckSum;
                            Clip->UsesRootMotion = Src->UsesRootMotion;
                            Clip->RootMotionNodeAnimID = 0;
                            if(Clip->UsesRootMotion){
                                Clip->RootMotionNodeAnimID = FileToIntegratedID(FileSource, 
                                                                                Src->RootMotionNodeAnimID);
                            }
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
    
    FillCorrespondingAssetArrays(Assets);
}