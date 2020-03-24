#include "tool_asset_build_commands.h"

#include <stdio.h>

// NOTE(Dima): Asset stuff
void BeginAsset(asset_system* System, u32 GroupID) {
	System->CurrentGroup = System->AssetGroups + GroupID;
	System->PrevAssetPointer = 0;
    
	game_asset_group* Group = System->CurrentGroup;
    
    // NOTE(Dima): Getting needed region
    game_asset_group_region NewRegion;
    
    NewRegion.FirstAssetIndex = System->AssetCount;
    NewRegion.AssetCount = 0;
    
    Group->Regions.push_back(NewRegion);
}

void EndAsset(asset_system* System) {
    Assert(System->CurrentGroup);
    game_asset_group* Group = System->CurrentGroup;
    
    System->CurrentGroup = 0;
    System->PrevAssetPointer = 0;
}

INTERNAL_FUNCTION added_asset AddAsset(asset_system* System, u32 AssetType) {
    added_asset Result = {};
    
    // NOTE(Dima): Getting needed group and region;
    Assert(System->CurrentGroup != 0);
    game_asset_group* Group = System->CurrentGroup;
    ASSERT(Group->Regions.size() > 0);
    
    game_asset_group_region* Region = &Group->Regions[Group->Regions.size() - 1];
    Region->AssetCount++;
    
    // NOTE(Dima): Setting needed data
    u32 AssetIndex = System->AssetCount;
    Result.Asset = System->Assets + AssetIndex;
    Result.Asset->Type = AssetType;
    Result.Source = System->AssetSources + AssetIndex;
    Result.Freearea = System->AssetFreeareas + AssetIndex;
    Result.FileHeader = System->FileHeaders + AssetIndex;
    Result.FileHeader->AssetType = AssetType;
    
    Result.Asset->ID = AssetIndex;
    Result.ID = AssetIndex;
    
    ++System->AssetCount;
    
    System->PrevAssetPointer = Result.Asset;
    
    return(Result);
}


INTERNAL_FUNCTION void 
AddFreeareaToAsset(asset_system* System, 
                   game_asset* Asset, 
                   void* Pointer) 
{
    game_asset_freearea* Free = System->AssetFreeareas + Asset->ID;
    
    int TargetFreeAreaIndex = Free->SetCount++;
    Assert(TargetFreeAreaIndex < FREEAREA_SLOTS_COUNT);
    
    Free->Pointers[TargetFreeAreaIndex] = Pointer;
}

INTERNAL_FUNCTION game_asset_tag* 
FindTagInAsset(game_asset* Asset, 
               u32 TagType) 
{
    game_asset_tag* Result = 0;
    
    for (int TagIndex = 0;
         TagIndex < Asset->Tags.size();
         TagIndex++)
    {
        game_asset_tag* Tag = &Asset->Tags[TagIndex];
        if (Tag->Type == TagType) {
            Result = Tag;
            break;
        }
    }
    
    return(Result);
}


game_asset_tag* AddTag(asset_system* System, u32 TagType, u32 TagValueType) {
    game_asset_tag* Result = 0;
    
    if (System->PrevAssetPointer) {
        /*
        NOTE(dima): First we should check if tag with
        the same type alredy exist.. Just for sure...
        */
        Result = FindTagInAsset(System->PrevAssetPointer, TagType);
        
        if (!Result) {
            System->PrevAssetPointer->Tags.push_back({});
            Result = &System->PrevAssetPointer->Tags[System->PrevAssetPointer->Tags.size() - 1];
            Result->Type = TagType;
            Result->ValueType = TagValueType;
        }
    }
    
    return(Result);
}

void AddFloatTag(asset_system* System, u32 TagType, float TagValue) {
    game_asset_tag* Tag = AddTag(System, TagType, AssetTagValue_Float);
    
    if (Tag) {
        Tag->Value_Float = TagValue;
    }
}

void AddIntTag(asset_system* System, u32 TagType, int TagValue) {
    game_asset_tag* Tag = AddTag(System, TagType, AssetTagValue_Int);
    
    if (Tag) {
        Tag->Value_Int = TagValue;
    }
}

void AddEmptyTag(asset_system* System, u32 TagType) {
    game_asset_tag* Tag = AddTag(System, TagType, AssetTagValue_Empty);
    
    if (Tag) {
        Tag->Value_Int = 1;
    }
}

void AddTagHubToAsset(asset_system* System, game_asset_tag_hub* TagHub){
    // NOTE(Dima): Adding font tags
    for(int TagIndex = 0; 
        TagIndex < TagHub->Tags.size();
        TagIndex++)
    {
        game_asset_tag* Tag = &TagHub->Tags[TagIndex];
        
        switch(TagHub->TagValueTypes[TagIndex]){
            case GameAssetTagValue_Float:{
                AddFloatTag(System, Tag->Type, Tag->Value_Float);
            }break;
            
            case GameAssetTagValue_Int:{
                AddIntTag(System, Tag->Type, Tag->Value_Int);
            }break;
            
            case GameAssetTagValue_Empty:{
                AddEmptyTag(System, Tag->Type);
            }break;
        }
    }
}

added_asset AddBitmapAsset(asset_system* System, char* Path, u32 BitmapLoadFlags) {
    added_asset Added = AddAsset(System, AssetType_Bitmap);
    
    // NOTE(Dima): Setting source
    game_asset_source* Source = Added.Source;
    
    Source->BitmapSource.Path = Path;
    Source->BitmapSource.BitmapInfo = 0;
    Source->BitmapSource.LoadFlags = BitmapLoadFlags;
    
    return(Added);
}

added_asset AddBitmapAssetManual(asset_system* System, 
                                 tool_bmp_info* Bitmap, 
                                 u32 BitmapLoadFlags) 
{
    added_asset Added = AddAsset(System, AssetType_Bitmap);
    
    asset_header* FileHeader = Added.FileHeader;
    
    // NOTE(Dima): Setting source
    game_asset_source* Source = Added.Source;
    Source->BitmapSource.BitmapInfo = Bitmap;
    Source->BitmapSource.Path = 0;
    Source->BitmapSource.LoadFlags = BitmapLoadFlags;
    
    return(Added);
}

added_asset AddIconAsset(asset_system* System,
                         char* Path)
{
    added_asset Result = AddBitmapAsset(System, Path, BitmapLoad_BakeIcon);
    
    return(Result);
}

added_asset AddSoundAsset(asset_system* System, 
                          char* Path) 
{
    added_asset Added = AddAsset(System, AssetType_Sound);
    
    // NOTE(Dima): Setting source
    game_asset_source* Source = Added.Source;
    Source->SoundSource.Path = Path;
    Source->SoundSource.Sound = 0;
    
    return(Added);
}

added_asset AddSoundAssetManual(asset_system* System, 
                                tool_sound_info* Sound)
{
    added_asset Added = AddAsset(System, AssetType_Sound);
    
    // NOTE(Dima): Setting source
    game_asset_source* Source = Added.Source;
    Source->SoundSource.Sound = Sound;
    Source->SoundSource.Path = 0;
    
    return(Added);
}


added_asset AddMeshAsset(asset_system* System, 
                         tool_mesh_info* Mesh) 
{
    added_asset Added = AddAsset(System, AssetType_Mesh);
    
    asset_header* FileHeader = Added.FileHeader;
    game_asset_source* Source = Added.Source;
    
    // NOTE(Dima): Setting source
    Source->MeshSource.MeshInfo = Mesh;
    
    // NOTE(Dima): Setting file header
    u32 VertexTypeSize = 0;
    if (Mesh->MeshType == Mesh_Simple) {
        FileHeader->Mesh.MeshType = Mesh_Simple;
        VertexTypeSize = sizeof(vertex_info);
    }
    else if(Mesh->MeshType == Mesh_Skinned) {
        FileHeader->Mesh.MeshType = Mesh_Skinned;
        VertexTypeSize = sizeof(vertex_skinned_info);
    }
    
    FileHeader->Mesh.VertexTypeSize = VertexTypeSize;
    FileHeader->Mesh.IndicesCount = sizeof(u32);
    FileHeader->Mesh.IndicesCount = Mesh->IndicesCount;
    FileHeader->Mesh.VerticesCount = Mesh->VerticesCount;
    
    FileHeader->Mesh.DataVerticesSize = FileHeader->Mesh.VertexTypeSize * Mesh->VerticesCount;
    FileHeader->Mesh.DataIndicesSize = sizeof(u32) * Mesh->IndicesCount;
    
    FileHeader->Mesh.DataOffsetToVertices = 0;
    FileHeader->Mesh.DataOffsetToIndices = FileHeader->Mesh.DataVerticesSize;
    
    AddFreeareaToAsset(System, Added.Asset, Mesh->Indices);
    AddFreeareaToAsset(System, Added.Asset, Mesh->Vertices);
    
    return(Added);
}

added_asset AddFontAsset(
asset_system* System,
tool_font_info* FontInfo)
{
    added_asset Added = AddAsset(System, AssetType_Font);
    
    game_asset_source* Source = Added.Source;
    asset_header* Header = Added.FileHeader;
    
    Source->FontSource.FontInfo = FontInfo;
    Added.Asset->Font = FontInfo;
    
    Header->Font.AscenderHeight = FontInfo->AscenderHeight;
    Header->Font.DescenderHeight = FontInfo->DescenderHeight;
    Header->Font.LineGap = FontInfo->LineGap;
    Header->Font.GlyphCount = FontInfo->GlyphCount;
    
    if(FontInfo->GlyphCount){
        AddFreeareaToAsset(System, Added.Asset, FontInfo->KerningPairs);
    }
    
    return(Added);
}

added_asset AddGlyphAssetInternal(
asset_system* System,
tool_glyph_info* GlyphInfo, 
u32 BitmapID)
{
    added_asset Added = AddAsset(System, AssetType_Glyph);
    
    game_asset_source* Source = Added.Source;
    Source->GlyphSource.Glyph = GlyphInfo;
    
    asset_header* Header = Added.FileHeader;
    Header->Glyph.Codepoint = GlyphInfo->Codepoint;
    Header->Glyph.BitmapWidth = GlyphInfo->Bitmap.Width;
    Header->Glyph.BitmapHeight = GlyphInfo->Bitmap.Height;
    Header->Glyph.BitmapWidthOverHeight = 
        (float)GlyphInfo->Bitmap.Width /
        (float)GlyphInfo->Bitmap.Height;
    Header->Glyph.XOffset = GlyphInfo->XOffset;
    Header->Glyph.YOffset = GlyphInfo->YOffset;
    Header->Glyph.Advance = GlyphInfo->Advance;
    Header->Glyph.LeftBearingX = GlyphInfo->LeftBearingX;
    Header->Glyph.BitmapID = BitmapID;
    
    return(Added);
}

added_asset AddGlyphAsset(asset_system* System,
                          tool_glyph_info* Glyph)
{
    added_asset BmpAsset = AddBitmapAssetManual(System, &Glyph->Bitmap, BitmapLoad_BakeIcon);
    added_asset Result = AddGlyphAssetInternal(System, Glyph, BmpAsset.ID);
    
    return(Result);
}

added_asset AddBitmapArray(asset_system* System, 
                           u32 FirstBitmapID, 
                           int Count)
{
    added_asset Added = AddAsset(System, AssetType_BitmapArray);
    
    asset_header* Header = Added.FileHeader;
    
    Header->BmpArray.Count = Count;
    Header->BmpArray.FirstBmpID = FirstBitmapID;
    
    return(Added);
}

void InitAssetFile(asset_system* Assets) {
    //NOTE(dima): Reserving first asset to make it NULL asset
    Assets->AssetCount = 1;
    Assets->PrevAssetPointer = 0;
    
    //NOTE(dima): Clearing asset groups
    for (int AssetGroupIndex = 0;
         AssetGroupIndex < GameAsset_Count;
         AssetGroupIndex++)
    {
        game_asset_group* Group = Assets->AssetGroups + AssetGroupIndex;
    }
    
    //NOTE(dima): Clearing free areas
    for (int FreeAreaIndex = 0;
         FreeAreaIndex < TEMP_STORED_ASSET_COUNT;
         FreeAreaIndex++) 
    {
        game_asset_freearea* Free = Assets->AssetFreeareas + FreeAreaIndex;
        
        *Free = {};
        
        Free->SetCount = 0;
        for (int PointerIndex = 0; PointerIndex < FREEAREA_SLOTS_COUNT; PointerIndex++) {
            Free->Pointers[PointerIndex] = 0;
        }
    }
}

void WriteAssetFile(asset_system* Assets, char* FileName) {
    FILE* fp = fopen(FileName, "wb");
    
    u32 AssetsLinesOffsetsCount = Assets->AssetCount - 1;
    u32 AssetsLinesOffsetsSize = sizeof(u32) * AssetsLinesOffsetsCount;
    u32* AssetsLinesOffsets = (u32*)malloc(AssetsLinesOffsetsSize);
    
    u32 AssetFileBytesWritten = 0;
    u32 AssetLinesBytesWritten = 0;
    if (fp) {
        
        //NOTE(dima): Writing asset file header
        asset_file_header FileHeader = {};
        
        FileHeader.Version = GetVersionInt(ASSET_FILE_VERSION_MAJOR,
                                           ASSET_FILE_VERSION_MINOR);
        FileHeader.EffectiveAssetsCount = Assets->AssetCount - 1;
        
        FileHeader.FileHeader[0] = 'J';
        FileHeader.FileHeader[1] = 'A';
        FileHeader.FileHeader[2] = 'S';
        FileHeader.FileHeader[3] = 'S';
        
        /*
NOTE(Dima): Calculating total number of regions in all assets.
And forming group regions that are about to be written
*/
        
        std::vector<asset_file_group> WriteGroups;
        std::vector<asset_file_group_region> WriteRegions;
        
        for (int GroupIndex = 0;
             GroupIndex < GameAsset_Count;
             GroupIndex++)
        {
            game_asset_group* Src = &Assets->AssetGroups[GroupIndex];
            asset_file_group Group;
            
            Group.FirstRegionIndex = 0;
            
            int RegionsCount = Src->Regions.size();
            if(RegionsCount){
                for(int i = 0; i < RegionsCount; i++)
                {
                    asset_file_group_region NewRegion;
                    
                    NewRegion.FirstAssetIndex = Src->Regions[i].FirstAssetIndex;
                    NewRegion.AssetCount = Src->Regions[i].AssetCount;
                    
                    if(i == 0){
                        Group.FirstRegionIndex = WriteRegions.size();
                    }
                    
                    WriteRegions.push_back(NewRegion);
                }
            }
            
            Group.RegionCount = RegionsCount;
            
            WriteGroups.push_back(Group);
        }
        
        int TotalGroupsCount = WriteGroups.size();
        ASSERT(TotalGroupsCount == GameAsset_Count);
        int TotalRegionsCount = WriteRegions.size();
        
        FileHeader.GroupsCount = TotalGroupsCount;
        FileHeader.RegionsCount = TotalRegionsCount;
        
        // NOTE(Dima): Writing header
        size_t HeaderBytesWritten = fwrite(&FileHeader, sizeof(asset_file_header), 1, fp);
        AssetFileBytesWritten += sizeof(asset_file_header);
        
        // NOTE(Dima): Writing groups that were previously formed
        fwrite(&WriteGroups[0], TotalGroupsCount * sizeof(asset_file_group), 1, fp);
        AssetFileBytesWritten += TotalGroupsCount * sizeof(asset_file_group);
        
        // NOTE(Dima): Writing regions that were previously formed
        fwrite(&WriteRegions[0], TotalRegionsCount * sizeof(asset_file_group_region), 1, fp);
        AssetFileBytesWritten += TotalRegionsCount * sizeof(asset_file_group_region);
        
        // NOTE(Dima): Writing assets
        for (int AssetIndex = 1;
             AssetIndex < Assets->AssetCount;
             AssetIndex++)
        {
            //NOTE(dima): Setting asset line offset
            AssetsLinesOffsets[AssetIndex - 1] = ftell(fp);
            
            game_asset* Asset = Assets->Assets + AssetIndex;
            game_asset_source* Source = Assets->AssetSources + AssetIndex;
            game_asset_freearea* Free = Assets->AssetFreeareas + AssetIndex;
            asset_header* Header = Assets->FileHeaders + AssetIndex;
            
            u32 HeaderByteSize = sizeof(asset_header);
            int AssetTagCount = Asset->Tags.size();
            u32 TagsByteSize =  AssetTagCount * sizeof(asset_tag_header);
            u32 DataByteSize = 0;
            
            /*
            NOTE(dima): Loading assets and setting assets
            headers data byte size.
            */
            switch (Asset->Type) {
                case AssetType_Bitmap: {
                    b32 BitmapAllocatedHere = 0;
                    if (!Source->BitmapSource.BitmapInfo) 
                    {
                        Asset->Bitmap = (tool_bmp_info*)malloc(sizeof(tool_bmp_info));
                        *Asset->Bitmap = LoadBMP(Source->BitmapSource.Path);
                        
                        BitmapAllocatedHere = 1;
                    }
                    else {
                        Asset->Bitmap = Source->BitmapSource.BitmapInfo;
                    }
                    AddFreeareaToAsset(Assets, Asset, Asset->Bitmap->Pixels);
                    
                    if (BitmapAllocatedHere) {
                        AddFreeareaToAsset(Assets, Asset, Asset->Bitmap);
                    }
                    
                    //NOTE(dima): Setting asset header
                    Header->Bitmap.Width = Asset->Bitmap->Width;
                    Header->Bitmap.Height = Asset->Bitmap->Height;
                    Header->Bitmap.BakeToAtlas = ((Source->BitmapSource.LoadFlags & 
                                                   BitmapLoad_BakeIcon) != 0);
                    
                    //NOTE(dima): Set data size
                    DataByteSize = Asset->Bitmap->Width * Asset->Bitmap->Height * 4;
                }break;
                
                case AssetType_Sound:{
                    b32 SoundAllocatedHere = 0;
                    if(!Source->SoundSource.Sound){
                        Asset->Sound = (tool_sound_info*)malloc(sizeof(tool_sound_info));
                        *Asset->Sound = LoadSound(Source->SoundSource.Path);
                        
                        SoundAllocatedHere = 1;
                    }
                    else{
                        Asset->Sound = Source->SoundSource.Sound;
                    }
                    AddFreeareaToAsset(Assets, Asset, Asset->Sound->Samples[0]);
                    
                    if(SoundAllocatedHere){
                        AddFreeareaToAsset(Assets, Asset, Asset->Sound);
                    }
                    
                    // NOTE(Dima): Setting asset header
                    Header->Sound.SampleCount = Asset->Sound->SampleCount;
                    Header->Sound.SamplesPerSec = Asset->Sound->SamplesPerSec;
                    Header->Sound.Channels = Asset->Sound->Channels;
                    
                    // NOTE(Dima): Setting data size
                    DataByteSize = Asset->Sound->SampleCount * Asset->Sound->Channels * sizeof(i16);
                }break;
                
                case AssetType_Font: {
                    Asset->Font = Source->FontSource.FontInfo;
                    
                    u32 SizeOfMapping = sizeof(int) * FONT_INFO_MAX_GLYPH_COUNT;
                    u32 SizeOfIDs = sizeof(u32) * Asset->Font->GlyphCount;
                    u32 SizeOfKerning = sizeof(float) * 
                        Asset->Font->GlyphCount * 
                        Asset->Font->GlyphCount;
                    
                    Header->Font.MappingSize = SizeOfMapping;
                    Header->Font.KerningSize = SizeOfKerning;
                    Header->Font.IDsSize = SizeOfIDs;
                    
                    Header->Font.DataOffsetToMapping = 0;
                    Header->Font.DataOffsetToKerning = SizeOfMapping;
                    Header->Font.DataOffsetToIDs = SizeOfMapping + SizeOfKerning;
                    
                    DataByteSize = SizeOfMapping + SizeOfKerning + SizeOfIDs;
                }break;
                
                case AssetType_Glyph: {
                    // NOTE(Dima): Nothing to write and set
                }break;
                
                case AssetType_Mesh: {
                    Asset->Mesh = Source->MeshSource.MeshInfo;
                    
                    DataByteSize = 
                        Header->Mesh.DataVerticesSize + 
                        Header->Mesh.DataIndicesSize;
                }break;
            }
            
            //NOTE(dima): Forming header
            Header->Pitch = HeaderByteSize + TagsByteSize + DataByteSize;
            Header->TagCount = AssetTagCount;
            Header->AssetType = Asset->Type;
            Header->TotalDataSize = DataByteSize;
            Header->TotalTagsSize = TagsByteSize;
            Header->LineTagOffset = sizeof(asset_header);
            Header->LineDataOffset = Header->LineTagOffset + TagsByteSize;
            
            std::vector<asset_tag_header> WriteTags;
            
            // NOTE(Dima): Forming tags in header
            for (int AssetTagIndex = 0;
                 AssetTagIndex < Header->TagCount;
                 AssetTagIndex++)
            {
                game_asset_tag* From = &Asset->Tags[AssetTagIndex];
                
                asset_tag_header NewTag;
                NewTag.Type = From->Type;
                NewTag.ValueType = From->ValueType;
                NewTag.Value.Value_Float = From->Value_Float;
                
                WriteTags.push_back(NewTag);
            }
            
            // NOTE(Dima): Writing asset header
            fwrite(Header, sizeof(asset_header), 1, fp);
            
            //NOTE(Dima): Writing tags
            fwrite(&WriteTags[0], TagsByteSize, 1, fp);
            
            //NOTE(dima): Writing asset data
            switch (Asset->Type) {
                case AssetType_Bitmap: {
                    //NOTE(dima): Writing bitmap pixel data
                    fwrite(Asset->Bitmap->Pixels, DataByteSize, 1, fp);
                }break;
                
                case AssetType_Sound:{
                    fwrite(Asset->Sound->Samples, DataByteSize, 1, fp);
                }break;
                
                case AssetType_Font: {
                    // IMPORTANT(Dima): Order is important
                    
                    // NOTE(Dima): FIRST - WRITING MAPPING
                    fwrite(
						Asset->Font->Codepoint2Glyph,
						Header->Font.MappingSize,
						1, fp);
                    
                    //NOTE(dima): SECOND - WRITING KERNING
                    fwrite(
                        Asset->Font->KerningPairs,
                        Header->Font.KerningSize,
                        1, fp);
                    
                    //NOTE(dima): THIRD - Write glyph IDs
                    fwrite(
                        Asset->Font->GlyphIDs,
                        Header->Font.IDsSize,
                        1, fp);
                    
                }break;
                
                case AssetType_Glyph: {
                    //NOTE(dima): Nothing to write
                }break;
                
                case AssetType_Mesh: {
                    //NOTE(dima): Writing vertices
                    fwrite(Asset->Mesh->Vertices,
                           Header->Mesh.DataVerticesSize, 
                           1, fp);
                    
                    //NOTE(dima): Writing indices
                    fwrite(
                        Asset->Mesh->Indices,
                        Header->Mesh.DataIndicesSize,
                        1, fp);
                }break;
            }
            
            //NOTE(dima): Freeing freareas
            for (int FreeIndex = 0; FreeIndex < Free->SetCount; FreeIndex++) {
                if(Free->Pointers[FreeIndex]){
                    free(Free->Pointers[FreeIndex]);
                    Free->Pointers[FreeIndex] = 0;
                }
            }
            
            //NOTE(dima): Incrementing file written data size
            AssetFileBytesWritten += Header->Pitch;
            AssetLinesBytesWritten += Header->Pitch;
        }
        
        fclose(fp);
    }
    else {
        INVALID_CODE_PATH;
    }
    
    
    
    //NOTE(dima): Reading file contents
    void* FileData = 0;
    fp = fopen(FileName, "rb");
    if (fp) {
        //NOTE(dima): Getting file size
        fseek(fp, 0, SEEK_END);
        u32 FileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        ASSERT(FileSize == AssetFileBytesWritten);
        
        //NOTE(dima): Reading file contents
        FileData = malloc(FileSize);
        fread(FileData, FileSize, 1, fp);
        
        fclose(fp);
    }
    else {
        INVALID_CODE_PATH;
    }
    
    //NOTE(dima): Incrementing asset lines offsets by size of asset lines offsets array
    for (int LineIndex = 0;
         LineIndex < AssetsLinesOffsetsCount;
         LineIndex++)
    {
        AssetsLinesOffsets[LineIndex] += AssetsLinesOffsetsSize;
    }
    
    //NOTE(dima): Inserting asset lines offsets after groups regions
    fp = fopen(FileName, "wb");
    if (fp) {
        asset_file_header* Header = (asset_file_header*)FileData;
        
        u32 GroupsByteSize = Header->GroupsCount * sizeof(asset_file_group);
        u32 GroupsRegionsSize = Header->RegionsCount * sizeof(asset_file_group_region);
        u32 LinesOffsetsSize = Header->EffectiveAssetsCount * sizeof(u32);
        u32 AssetsLinesByteSize = AssetLinesBytesWritten;
        
        u32 GroupsByteOffset = sizeof(asset_file_header);
        u32 GroupsRegionsByteOffset = GroupsByteOffset + GroupsByteSize;
        u32 LinesOffsetsByteOffset = GroupsRegionsByteOffset + GroupsRegionsSize;
        u32 AssetLinesByteOffset = LinesOffsetsByteOffset + LinesOffsetsSize;
        
        Header->GroupsByteOffset = GroupsByteOffset;
        Header->GroupsRegionsByteOffset = GroupsRegionsByteOffset;
        Header->LinesOffsetsByteOffset = LinesOffsetsByteOffset;
        Header->AssetLinesByteOffset = AssetLinesByteOffset;
        
        //NOTE(dima): Rewriting header
        fwrite(Header, sizeof(asset_file_header), 1, fp);
        
        //NOTE(dima): Rewriting groups
        ASSERT(GroupsByteOffset == ftell(fp));
        fwrite((u8*)FileData + GroupsByteOffset, GroupsByteSize, 1, fp);
        
        // NOTE(Dima): Rewriting groups regions
        ASSERT(GroupsRegionsByteOffset == ftell(fp));
        fwrite((u8*)FileData + GroupsRegionsByteOffset, GroupsRegionsSize, 1, fp);
        
        //NOTE(dima): Writing asset lines offsets
        ASSERT(LinesOffsetsByteOffset == ftell(fp));
        fwrite(AssetsLinesOffsets, AssetsLinesOffsetsSize, 1, fp);
        
        //NOTE(dima): Rewriting asset data lines
        ASSERT(AssetLinesByteOffset == ftell(fp));
        fwrite(
            (u8*)FileData + GroupsRegionsByteOffset + GroupsRegionsSize, 
            AssetLinesBytesWritten, 1, fp);
    }
    else {
        INVALID_CODE_PATH;
    }
    
    if (FileData) {
        free(FileData);
    }
    
    free(AssetsLinesOffsets);
    
    printf("File %s has been written...\n", FileName);
}