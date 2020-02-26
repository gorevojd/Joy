#include "tool_asset_build.h"
#include <stdio.h>

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

INTERNAL_FUNCTION void BeginAsset(asset_system* System, u32 GroupID) {
	System->CurrentGroup = System->AssetGroups + GroupID;
	System->PrevAssetPointer = 0;
    
	game_asset_group* Group = System->CurrentGroup;
    
    // NOTE(Dima): Getting needed region
    int RegionIndex = Group->RegionsCount++;
    ASSERT(RegionIndex < ASSET_GROUP_REGIONS_COUNT);
    game_asset_group_region* Region = &Group->Regions[RegionIndex];
    
    Region->FirstAssetIndex = System->AssetCount;
    Region->AssetCount = 0;
}

INTERNAL_FUNCTION void EndAsset(asset_system* System) {
	Assert(System->CurrentGroup);
	game_asset_group* Group = System->CurrentGroup;
    
	System->CurrentGroup = 0;
	System->PrevAssetPointer = 0;
}

struct added_asset {
	game_asset* Asset;
	game_asset_source* Source;
	game_asset_freearea* Freearea;
	asset_header* FileHeader;
    u32 ID;
};

INTERNAL_FUNCTION added_asset AddAsset(asset_system* System, u32 AssetType) {
	added_asset Result = {};
    
    // NOTE(Dima): Getting needed group and region;
	Assert(System->CurrentGroup != 0);
	game_asset_group* Group = System->CurrentGroup;
    ASSERT(Group->RegionsCount > 0);
    
    game_asset_group_region* Region = &Group->Regions[Group->RegionsCount - 1];
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

INTERNAL_FUNCTION void AddFreeareaToAsset(asset_system* System, game_asset* Asset, void* Pointer) {
	game_asset_freearea* Free = System->AssetFreeareas + Asset->ID;
    
	int TargetFreeAreaIndex = Free->SetCount++;
	Assert(TargetFreeAreaIndex < FREEAREA_SLOTS_COUNT);
    
	Free->Pointers[TargetFreeAreaIndex] = Pointer;
}

inline game_asset_tag* FindTagInAsset(game_asset* Asset, u32 TagType) {
	game_asset_tag* Result = 0;
    
	for (int TagIndex = 0;
         TagIndex < Asset->TagCount;
         TagIndex++)
	{
		game_asset_tag* Tag = Asset->Tags + TagIndex;
		if (Tag->Type == TagType) {
			Result = Tag;
			break;
		}
	}
    
	return(Result);
}

INTERNAL_FUNCTION game_asset_tag* AddTag(asset_system* System, u32 TagType) {
	game_asset_tag* Result = 0;
    
	if (System->PrevAssetPointer) {
		/*
  NOTE(dima): First we should check if tag with
  the same type alredy exist.. Just for sure...
  */
		Result = FindTagInAsset(System->PrevAssetPointer, TagType);
        
		if (!Result) {
			if (System->PrevAssetPointer->TagCount < MAX_TAGS_PER_ASSET - 1) {
				//NOTE(dima): Getting tag and incrementing tag count
				Result = System->PrevAssetPointer->Tags + System->PrevAssetPointer->TagCount++;
				Result->Type = TagType;
			}
		}
	}
    
	return(Result);
}

INTERNAL_FUNCTION void AddFloatTag(asset_system* System, u32 TagType, float TagValue) {
	game_asset_tag* Tag = AddTag(System, TagType);
    
	if (Tag) {
		Tag->Value_Float = TagValue;
	}
}

INTERNAL_FUNCTION void AddIntTag(asset_system* System, u32 TagType, int TagValue) {
	game_asset_tag* Tag = AddTag(System, TagType);
    
	if (Tag) {
		Tag->Value_Int = TagValue;
	}
}

INTERNAL_FUNCTION void AddEmptyTag(asset_system* System, u32 TagType) {
	game_asset_tag* Tag = AddTag(System, TagType);
    
	if (Tag) {
		Tag->Value_Int = 1;
	}
}

INTERNAL_FUNCTION added_asset AddBitmapAsset(asset_system* System, char* Path, u32 BitmapLoadFlags = 0) {
	added_asset Added = AddAsset(System, AssetType_Bitmap);
    
    // NOTE(Dima): Setting source
	game_asset_source* Source = Added.Source;
    
	Source->BitmapSource.Path = Path;
	Source->BitmapSource.BitmapInfo = 0;
    Source->BitmapSource.LoadFlags = BitmapLoadFlags;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddBitmapAssetManual(asset_system* System, 
                                                   bmp_info* Bitmap, 
                                                   u32 BitmapLoadFlags = 0) 
{
	added_asset Added = AddAsset(System, AssetType_Bitmap);
    
	asset_header* FileHeader = Added.FileHeader;
    
    // NOTE(Dima): Setting source
	game_asset_source* Source = Added.Source;
	Source->BitmapSource.BitmapInfo = Bitmap;
    Source->BitmapSource.Path = 0;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddIconAsset(asset_system* System,
                                           char* Path)
{
    added_asset Result = AddBitmapAsset(System, Path, BitmapLoad_BakeIcon);
    
    return(Result);
}

INTERNAL_FUNCTION added_asset AddSoundAsset(asset_system* System, 
                                            char* Path,
                                            u32 BitmapLoadFlags = 0) 
{
	added_asset Added = AddAsset(System, AssetType_Sound);
    
    // NOTE(Dima): Setting source
	game_asset_source* Source = Added.Source;
	Source->SoundSource.Path = Path;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddModelAsset(asset_system* System, 
                                            u32 MeshID, 
                                            u32 MaterialID) 
{
	added_asset Added = AddAsset(System, AssetType_Model);
    
	game_asset_source* Source = Added.Source;
    asset_header* FileHeader = Added.FileHeader;
    
    // NOTE(Dima): Setting source
    
    
    // NOTE(Dima): Setting file header
    FileHeader->Model.MeshID = MeshID;
    FileHeader->Model.MaterialID = MaterialID;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddMeshAsset(asset_system* System, 
                                           mesh_info* Mesh) 
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
	FileHeader->Mesh.LineOffsetToVertices = GetLineOffsetForData();
	FileHeader->Mesh.LineOffsetToIndices = FileHeader->Mesh.LineOffsetToVertices +
		Mesh->VerticesCount * VertexTypeSize;
    
	AddFreeareaToAsset(System, Added.Asset, Mesh->Indices);
	AddFreeareaToAsset(System, Added.Asset, Mesh->Vertices);
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddFontAsset(
asset_system* System,
font_info* FontInfo,
u32 FirstGlyphID)
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
    Header->Font.FirstGlyphID = FirstGlyphID;
    
	Header->Font.LineOffsetToKerningPairs= GetLineOffsetForData();
	
    AddFreeareaToAsset(System, Added.Asset, FontInfo->KerningPairs);
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddGlyphAssetInternal(
asset_system* System,
glyph_info* GlyphInfo, 
u32 BitmapID)
{
    added_asset Added = AddAsset(System, AssetType_Glyph);
    
	game_asset_source* Source = Added.Source;
	Source->GlyphSource.Glyph = GlyphInfo;
    
	asset_header* Header = Added.FileHeader;
	Header->Glyph.Codepoint = GlyphInfo->Codepoint;
	Header->Glyph.BitmapWidth = GlyphInfo->Bitmap.Width;
	Header->Glyph.BitmapHeight = GlyphInfo->Bitmap.Height;
	Header->Glyph.XOffset = GlyphInfo->XOffset;
	Header->Glyph.YOffset = GlyphInfo->YOffset;
	Header->Glyph.Advance = GlyphInfo->Advance;
	Header->Glyph.LeftBearingX = GlyphInfo->LeftBearingX;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddGlyphAsset(asset_system* System,
                                            glyph_info* Glyph)
{
    added_asset BmpAsset = AddBitmapAssetManual(System, &Glyph->Bitmap, BitmapLoad_BakeIcon);
    added_asset Result = AddGlyphAssetInternal(System, Glyph, BmpAsset.ID);
    
    return(Result);
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
        
        for(int i = 0; i < ASSET_GROUP_REGIONS_COUNT; i++)
        {
            Group->Regions[i].FirstAssetIndex = 0;
            Group->Regions[i].AssetCount = 0;
        }
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
		FileHeader.GroupsCount = GameAsset_Count;
		FileHeader.EffectiveAssetsCount = Assets->AssetCount - 1;
        
		FileHeader.FileHeader[0] = 'J';
		FileHeader.FileHeader[1] = 'A';
		FileHeader.FileHeader[2] = 'S';
		FileHeader.FileHeader[3] = 'S';
        
		size_t HeaderBytesWritten = fwrite(&FileHeader, sizeof(asset_file_header), 1, fp);
        AssetFileBytesWritten += sizeof(asset_file_header);
        
        //NOTE(dima): Writing asset groups after asset file header
        for (int GroupIndex = 0;
             GroupIndex < FileHeader.GroupsCount;
             GroupIndex++)
        {
            game_asset_group* Src = &Assets->AssetGroups[GroupIndex];
            asset_file_group Group;
            
            // NOTE(Dima): Copy all groups and regions
            int i;
            for(i = 0; i < Src->RegionsCount; i++)
            {
                Group.Regions[i].FirstAssetIndex = Src->Regions[i].FirstAssetIndex;
                Group.Regions[i].AssetCount = Src->Regions[i].AssetCount;
            }
            // NOTE(Dima): Set unnesessary to NULL
            for(i; i < ASSET_GROUP_REGIONS_COUNT; i++){
                Group.Regions[i].FirstAssetIndex = 0;
                Group.Regions[i].AssetCount = 0;
            }
            
            fwrite(&Group, sizeof(asset_file_group), 1, fp);
            AssetFileBytesWritten += sizeof(asset_file_group);
        }
        
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
            u32 TagsByteSize = Asset->TagCount * sizeof(asset_tag_header);
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
                        Asset->Bitmap = (bmp_info*)malloc(sizeof(bmp_info));
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
                    Header->Bitmap.BakeToAtlas = ((Source->BitmapSource.LoadFlags & BitmapLoad_BakeIcon) != 0);
                    
                    //NOTE(dima): Set data size
                    DataByteSize = Asset->Bitmap->Width * Asset->Bitmap->Height * 4;
                }break;
                
                case AssetType_Font: {
                    Asset->Font = Source->FontSource.FontInfo;
                    
                    u32 SizeOfMapping = sizeof(int) * FONT_INFO_MAX_GLYPH_COUNT;
                    u32 SizeOfKerning = sizeof(float) * 
                        Asset->Font->GlyphCount * 
                        Asset->Font->GlyphCount;
                    
                    Header->Font.LineOffsetToMapping = GetLineOffsetForData();
                    Header->Font.LineOffsetToKerningPairs = Header->Font.LineOffsetToMapping + SizeOfMapping;
                    
                    DataByteSize = SizeOfMapping + SizeOfKerning;
                }break;
                
                case AssetType_Glyph: {
                    // NOTE(Dima): Nothing to write and set
                }break;
                
                case AssetType_Mesh: {
                    Asset->Mesh = Source->MeshSource.MeshInfo;
                    
                    DataByteSize = 
                        Header->Mesh.VertexTypeSize * Asset->Mesh->VerticesCount + 
                        sizeof(u32) * Asset->Mesh->IndicesCount;
                }break;
            }
            
            //NOTE(dima): Forming and writing header
            Header->LineDataOffset = GetLineOffsetForData();
            Header->Pitch = HeaderByteSize + DataByteSize + TagsByteSize;
            Header->LineFirstTagOffset = HeaderByteSize + DataByteSize;
            Header->TagCount = Asset->TagCount;
            Header->AssetType = Asset->Type;
            Header->TotalDataSize = DataByteSize;
            Header->TotalTagsSize = TagsByteSize;
            
            ASSERT(Header->TotalTagsSize == Asset->TagCount * sizeof(asset_tag_header));
            
            fwrite(Header, sizeof(asset_header), 1, fp);
            
            //NOTE(dima): Writing asset data
            switch (Asset->Type) {
                case AssetType_Bitmap: {
                    //NOTE(dima): Writing bitmap pixel data
                    fwrite(Asset->Bitmap->Pixels, DataByteSize, 1, fp);
                }break;
                
                case AssetType_Font: {
                    //NOTE(dima): Writing mapping data
                    fwrite(
						Asset->Font->Codepoint2Glyph,
						sizeof(int) * FONT_INFO_MAX_GLYPH_COUNT,
						1, fp);
                    
                    //NOTE(dima): Writing kerning pairs
                    fwrite(
						Asset->Font->KerningPairs,
						sizeof(float) * Asset->Font->GlyphCount * Asset->Font->GlyphCount,
						1, fp);
                }break;
                
                case AssetType_Glyph: {
                    //NOTE(dima): Nothing to write
                }break;
                
                case AssetType_Mesh: {
                    //NOTE(dima): Writing vertices
                    fwrite(Asset->Mesh->Vertices,
                           Asset->Mesh->VerticesCount * 
                           Header->Mesh.VertexTypeSize, 
                           1, fp);
                    
                    //NOTE(dima): Writing indices
                    fwrite(
                        Asset->Mesh->Indices,
                        Asset->Mesh->IndicesCount * sizeof(u32),
                        1, fp);
                }break;
            }
            
            //NOTE(dima): Forming tags
            asset_tag_header WriteTags[MAX_TAGS_PER_ASSET];
            
            for (int AssetTagIndex = 0;
                 AssetTagIndex < Asset->TagCount;
                 AssetTagIndex++)
            {
                game_asset_tag* From = Asset->Tags + AssetTagIndex;
                asset_tag_header* To = WriteTags + AssetTagIndex;
                
                To->Type = From->Type;
                To->Value_Float = From->Value_Float;
            }
            fwrite(WriteTags, TagsByteSize, 1, fp);
            
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
    
    //NOTE(dima): Inserting asset lines offsets after groups
    fp = fopen(FileName, "wb");
    if (fp) {
        asset_file_header* Header = (asset_file_header*)FileData;
        
        u32 GroupsByteSize = Header->GroupsCount * sizeof(asset_file_group);
        u32 LinesOffsetsSize = Header->EffectiveAssetsCount * sizeof(u32);
        u32 AssetsLinesByteSize = AssetLinesBytesWritten;
        
        u32 GroupsByteOffset = sizeof(asset_file_header);
        u32 LinesOffsetsByteOffset = GroupsByteOffset + GroupsByteSize;
        u32 AssetLinesByteOffset = LinesOffsetsByteOffset + LinesOffsetsSize;
        
        Header->GroupsByteOffset = GroupsByteOffset;
        Header->LinesOffsetsByteOffset = LinesOffsetsByteOffset;
        Header->AssetLinesByteOffset = AssetLinesByteOffset;
        
        //NOTE(dima): Rewriting header
        fwrite(Header, sizeof(asset_file_header), 1, fp);
        
        //NOTE(dima): Rewriting groups
        ASSERT(GroupsByteOffset == ftell(fp));
        fwrite((u8*)FileData + GroupsByteOffset, GroupsByteSize, 1, fp);
        
        //NOTE(dima): Writing asset lines offsets
        ASSERT(LinesOffsetsByteOffset == ftell(fp));
        fwrite(AssetsLinesOffsets, AssetsLinesOffsetsSize, 1, fp);
        
        //NOTE(dima): Rewriting asset data lines
        ASSERT(AssetLinesByteOffset == ftell(fp));
        fwrite(
            (u8*)FileData + GroupsByteOffset + GroupsByteSize, 
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

INTERNAL_FUNCTION void WriteFontsChunk(font_info* Fonts,
                                       u32* FontsGroups,
                                       game_asset_tag_hub* FontsTags,
                                       int FontsCount, 
                                       int ChunkIndex)
{
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    u32 FirstGlyphIDsForFonts[ASSET_GROUP_REGIONS_COUNT];
    
    // NOTE(Dima): Adding glyphs
    BeginAsset(System, GameAsset_Type_Glyph);
    for (int FontIndex = 0;
         FontIndex < FontsCount;
         FontIndex++)
    {
        font_info* Font = &Fonts[FontIndex];
        
        for (int GlyphIndex = 0;
             GlyphIndex < Font->GlyphCount;
             GlyphIndex++)
        {
            added_asset AddedGlyphAsset = AddGlyphAsset(System, &Font->Glyphs[GlyphIndex]);
            
            if (GlyphIndex == 0) {
                u32 AddedGlyphID = AddedGlyphAsset.ID;
                FirstGlyphIDsForFonts[FontIndex] = AddedGlyphID;
            }
        }
    }
    EndAsset(System);
    
    // NOTE(Dima): Adding fonts
    for (int FontIndex = 0;
         FontIndex < FontsCount;
         FontIndex++)
    {
        font_info* Font = &Fonts[FontIndex];
        game_asset_tag_hub* TagHub = FontsTags + FontIndex;
        
        BeginAsset(System, FontsGroups[FontIndex]);
        // NOTE(Dima): Adding font asset
        AddFontAsset(System, Font, FirstGlyphIDsForFonts[FontIndex]);
        // NOTE(Dima): Adding font tags
        for(int TagIndex = 0; 
            TagIndex < TagHub->TagCount;
            TagIndex++)
        {
            game_asset_tag* Tag = TagHub->Tags + TagIndex;
            
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
        EndAsset(System);
    }
    
    // NOTE(Dima): Writing file
    char OutFileName[256];
    stbsp_sprintf(OutFileName, "../Data/Fonts%d.ja", ChunkIndex);
    
    WriteAssetFile(System, OutFileName);
}

INTERNAL_FUNCTION void WriteFonts(){
    font_info LibMono = LoadFont("../Data/Fonts/LiberationMono-Regular.ttf", 18.0f, LoadFont_BakeShadow);
    font_info Lilita = LoadFont("../Data/Fonts/LilitaOne.ttf", 20.0f, LoadFont_BakeShadow);
    font_info Inconsolata = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 18.0f, LoadFont_BakeBlur);
    font_info PFDIN = LoadFont("../Data/Fonts/PFDinTextCondPro-Regular.ttf", 18.0f, LoadFont_BakeBlur);
    font_info MollyJack = LoadFont("../Data/Fonts/MollyJack.otf", 40.0f, LoadFont_BakeBlur);
    
    font_info* Fonts[] = {
        &LibMono,
        &Lilita,
        &Inconsolata,
        &PFDIN,
        &MollyJack,
    };
    
    u32 Groups[] = {
        GameAsset_LiberationMono,
        GameAsset_LilitaOne,
        GameAsset_Inconsolata,
        GameAsset_PFDIN,
        GameAsset_MollyJackFont,
    };
    
    game_asset_tag_hub Tags[] = {
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Bold),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
    };
    
    int FontCount = ArrayCount(Fonts);
    
    int ChunkIndex = 0;
    for(int FontIndex = 0;
        FontIndex < FontCount;
        FontIndex += ASSET_GROUP_REGIONS_COUNT,
        ChunkIndex++)
    {
        font_info* CurFontsChunk = Fonts[FontIndex];
        u32 * CurChunkGroups = Groups + FontIndex;
        game_asset_tag_hub* CurChunkTagHub = Tags + FontIndex;
        
        int CurChunkSize = Min(FontCount - FontIndex, ASSET_GROUP_REGIONS_COUNT);
        
        if(CurChunkSize){
            WriteFontsChunk(CurFontsChunk, 
                            CurChunkGroups,
                            CurChunkTagHub,
                            CurChunkSize, 
                            ChunkIndex);
        }
    }
}

INTERNAL_FUNCTION void WriteIcons(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    BeginAsset(System, GameAsset_CheckboxMark);
    AddIconAsset(System, "../Data/Icons/checkmark64.png");
    EndAsset(System);
    
    BeginAsset(System, GameAsset_ChamomileIcon);
    AddIconAsset(System, "../Data/Icons/chamomile.png");
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/Icons.ja");
}

INTERNAL_FUNCTION void WriteSounds(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    BeginAsset(System, GameAsset_CheckboxMark);
    EndAsset(System);
    
    BeginAsset(System, GameAsset_ChamomileIcon);
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/GeneratedSounds.ja");
}

INTERNAL_FUNCTION void WriteBitmaps(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    
    
    WriteAssetFile(System, "../Data/Bitmaps.ja");
}

INTERNAL_FUNCTION void WriteMeshPrimitives(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    mesh_info Cube = MakeCube();
    mesh_info Plane = MakePlane();
    
    mesh_info SphereMeshSuperHig = MakeSphere(160, 80);
    mesh_info SphereMeshHig = MakeSphere(80, 40);
    mesh_info SphereMeshAvg = MakeSphere(40, 20);
    mesh_info SphereMeshLow = MakeSphere(20, 10);
    mesh_info SphereMeshSuperLow = MakeSphere(10, 5);
    
    mesh_info CylMeshSuperLow = MakeCylynder(2.0f, 0.5f, 6);
    mesh_info CylMeshLow = MakeCylynder(2.0f, 0.5f, 12);
    mesh_info CylMeshAvg = MakeCylynder(2.0f, 0.5f, 24);
    mesh_info CylMeshHig = MakeCylynder(2.0f, 0.5f, 48);
    mesh_info CylMeshSuperHig = MakeCylynder(2.0f, 0.5f, 96);
    
    BeginAsset(System, GameAsset_Cube);
    AddMeshAsset(System, &Cube);
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Plane);
    AddMeshAsset(System, &Plane);
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Sphere);
    AddMeshAsset(System, &SphereMeshSuperLow);
    AddFloatTag(System, AssetTag_LOD, 0.0f);
    AddMeshAsset(System, &SphereMeshLow);
    AddFloatTag(System, AssetTag_LOD, 0.25f);
    AddMeshAsset(System, &SphereMeshAvg);
    AddFloatTag(System, AssetTag_LOD, 0.5f);
    AddMeshAsset(System, &SphereMeshHig);
    AddFloatTag(System, AssetTag_LOD, 0.75f);
    AddMeshAsset(System, &SphereMeshSuperHig);
    AddFloatTag(System, AssetTag_LOD, 1.0f);
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Cylynder);
    AddMeshAsset(System, &CylMeshSuperLow);
    AddFloatTag(System, AssetTag_LOD, 0.0f);
    AddMeshAsset(System, &CylMeshLow);
    AddFloatTag(System, AssetTag_LOD, 0.25f);
    AddMeshAsset(System, &CylMeshAvg);
    AddFloatTag(System, AssetTag_LOD, 0.5f);
    AddMeshAsset(System, &CylMeshHig);
    AddFloatTag(System, AssetTag_LOD, 0.75f);
    AddMeshAsset(System, &CylMeshSuperHig);
    AddFloatTag(System, AssetTag_LOD, 1.0f);
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/MeshPrimitives.ja");
}

int main() {
    WriteFonts();
    WriteBitmaps();
    WriteIcons();
    WriteMeshPrimitives();
    
    system("pause");
    return(0);
}