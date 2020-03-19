#include "tool_asset_build.h"

#include "joy_software_renderer.h"
#include "joy_render_blur.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <set>

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"


#define STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_STATIC
#include "stb_truetype.h"


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
    Source->BitmapSource.LoadFlags = BitmapLoadFlags;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddIconAsset(asset_system* System,
                                           char* Path)
{
    added_asset Result = AddBitmapAsset(System, Path, BitmapLoad_BakeIcon);
    
    return(Result);
}

INTERNAL_FUNCTION added_asset AddSoundAsset(asset_system* System, 
                                            char* Path) 
{
	added_asset Added = AddAsset(System, AssetType_Sound);
    
    // NOTE(Dima): Setting source
	game_asset_source* Source = Added.Source;
	Source->SoundSource.Path = Path;
    Source->SoundSource.Sound = 0;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddSoundAssetManual(asset_system* System, 
                                                  sound_info* Sound)
{
    added_asset Added = AddAsset(System, AssetType_Sound);
    
    // NOTE(Dima): Setting source
	game_asset_source* Source = Added.Source;
	Source->SoundSource.Sound = Sound;
    Source->SoundSource.Path = 0;
    
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
    
    FileHeader->Mesh.DataVerticesSize = FileHeader->Mesh.VertexTypeSize * Mesh->VerticesCount;
    FileHeader->Mesh.DataIndicesSize = sizeof(u32) * Mesh->IndicesCount;
    
	FileHeader->Mesh.DataOffsetToVertices = 0;
	FileHeader->Mesh.DataOffsetToIndices = FileHeader->Mesh.DataVerticesSize;
    
	AddFreeareaToAsset(System, Added.Asset, Mesh->Indices);
	AddFreeareaToAsset(System, Added.Asset, Mesh->Vertices);
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddFontAsset(
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

INTERNAL_FUNCTION added_asset AddGlyphAssetInternal(
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

INTERNAL_FUNCTION added_asset AddGlyphAsset(asset_system* System,
                                            tool_glyph_info* Glyph)
{
    added_asset BmpAsset = AddBitmapAssetManual(System, &Glyph->Bitmap, BitmapLoad_BakeIcon);
    added_asset Result = AddGlyphAssetInternal(System, Glyph, BmpAsset.ID);
    
    return(Result);
}

INTERNAL_FUNCTION added_asset AddBitmapArray(asset_system* System, 
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


void LoadFontAddCodepoint(
tool_font_info* FontInfo, 
stbtt_fontinfo* STBFont,
int Codepoint,
int* AtlasWidth, 
int* AtlasHeight,
float FontScale,
u32 Flags,
int BlurRadius,
float* GaussianBox)
{
    int glyphIndex = FontInfo->GlyphCount++;
    tool_glyph_info* glyph = &FontInfo->Glyphs[glyphIndex];
    
    FontInfo->Codepoint2Glyph[Codepoint] = glyphIndex;
    
    int CharWidth;
	int CharHeight;
	int XOffset;
	int YOffset;
	int Advance;
	int LeftBearingX;
    
    u8* Bitmap = stbtt_GetCodepointBitmap(
        STBFont,
        FontScale, FontScale,
        Codepoint,
        &CharWidth, &CharHeight,
        &XOffset, &YOffset);
    
    if(CharWidth > 9999){
        CharWidth = 0;
    }
    if(CharHeight > 9999){
        CharHeight = 0;
    }
    
    int CharBorder = 3;
    int ShadowOffset = 0;
    
    if(Flags & LoadFont_BakeShadow){
        ShadowOffset = 2;
    }
    
    int CharBmpWidth= CharBorder * 2 + CharWidth;
    int CharBmpHeight = CharBorder * 2 + CharHeight;
    
    stbtt_GetCodepointHMetrics(STBFont, Codepoint, &Advance, &LeftBearingX);
    
    glyph->Width = CharBmpWidth + ShadowOffset;
	glyph->Height = CharBmpHeight + ShadowOffset;
	glyph->Bitmap = AllocateBitmap(glyph->Width, glyph->Height);
	glyph->Advance = Advance * FontScale;
	glyph->LeftBearingX = LeftBearingX * FontScale;
	glyph->XOffset = XOffset - CharBorder;
	glyph->YOffset = YOffset - CharBorder;
	glyph->Codepoint = Codepoint;
    
    *AtlasWidth += glyph->Width;
	*AtlasHeight = Max(*AtlasHeight, glyph->Height);
    
    //NOTE(dima): Clearing the image bytes
	u32* Pixel = (u32*)glyph->Bitmap.Pixels;
	for (int Y = 0; Y < glyph->Height; Y++) {
		for (int X = 0; X < glyph->Width; X++) {
			*Pixel++ = 0;
		}
	}
    
    //NOTE(dima): Forming char bitmap
	float OneOver255 = 1.0f / 255.0f;
    
    bmp_info CharBitmap= AllocateBitmap(CharWidth, CharHeight);
	
	for (int j = 0; j < CharHeight; j++) {
		for (int i = 0; i < CharWidth; i++) {
			u8 Grayscale = *((u8*)Bitmap + j * CharWidth + i);
			float Grayscale01 = (float)Grayscale * OneOver255;
            
			v4 resColor = V4(1.0f, 1.0f, 1.0f, Grayscale01);
            
			/*Alpha premultiplication*/
			resColor.r *= resColor.a;
			resColor.g *= resColor.a;
			resColor.b *= resColor.a;
            
			u32 ColorValue = PackRGBA(resColor);
			u32* TargetPixel = (u32*)((u8*)CharBitmap.Pixels + j* CharBitmap.Pitch + i * 4);
			*TargetPixel = ColorValue;
		}
	}
    
    //NOTE(dima): Render blur if needed
	if (Flags & LoadFont_BakeBlur) {
		bmp_info ToBlur = AllocateBitmap(
            2 * CharBorder + CharWidth,
            2 * CharBorder + CharHeight);
        
		bmp_info BlurredResult = AllocateBitmap(
            2 * CharBorder + CharWidth,
            2 * CharBorder + CharHeight);
        
		bmp_info TempBitmap = AllocateBitmap(
            2 * CharBorder + CharWidth,
            2 * CharBorder + CharHeight);
        
		RenderOneBitmapIntoAnother(
            &ToBlur,
            &CharBitmap,
            CharBorder,
            CharBorder,
            V4(1.0f, 1.0f, 1.0f, 1.0f));
        
#if 1
		BlurBitmapExactGaussian(
            &ToBlur,
            BlurredResult.Pixels,
            ToBlur.Width,
            ToBlur.Height,
            BlurRadius,
            GaussianBox);
        
        
		for (int Y = 0; Y < ToBlur.Height; Y++) {
			for (int X = 0; X < ToBlur.Width; X++) {
				u32* FromPix = (u32*)BlurredResult.Pixels + Y * BlurredResult.Width + X;
				u32* ToPix = (u32*)ToBlur.Pixels + Y * ToBlur.Width + X;
                
				v4 FromColor = UnpackRGBA(*FromPix);
                
				v4 resColor = FromColor;
				if (resColor.a < 0.15f) {
                    resColor.a = 0.0f;
				}
                else{
                    resColor.a = 1.0f;
                }
                
				*ToPix = PackRGBA(resColor);
			}
		}
        
		BlurBitmapExactGaussian(
            &ToBlur,
            BlurredResult.Pixels,
            ToBlur.Width,
            ToBlur.Height,
            BlurRadius,
            GaussianBox);
        
#else
		BlurBitmapApproximateGaussian(
            &ToBlur,
            BlurredResult.Pixels,
            TempBitmap.Pixels,
            ToBlur.Width,
            ToBlur.Height,
            BlurRadius);
		for (int Y = 0; Y < ToBlur.Height; Y++) {
			for (int X = 0; X < ToBlur.Width; X++) {
				u32* FromPix = (u32*)BlurredResult.Pixels + Y * BlurredResult.Width + X;
				u32* ToPix = (u32*)ToBlur.Pixels + Y * ToBlur.Width + X;
				v4 FromColor = UnpackRGBA(*FromPix);
				v4 resColor = FromColor;
				if (resColor.a > 0.05f) {
					resColor.a = 1.0f;
				}
				*ToPix = PackRGBA(resColor);
			}
		}
		BlurBitmapApproximateGaussian(
            &ToBlur,
            BlurredResult.Pixels,
            TempBitmap.Pixels,
            ToBlur.Width,
            ToBlur.Height,
            BlurRadius);
#endif
        
		RenderOneBitmapIntoAnother(
            &glyph->Bitmap,
            &BlurredResult,
            0, 0,
            V4(0.0f, 0.0f, 0.0f, 1.0f));
        
		DeallocateBitmap(&TempBitmap);
		DeallocateBitmap(&ToBlur);
		DeallocateBitmap(&BlurredResult);
	}
    
    if(Flags & LoadFont_BakeShadow){
        
        RenderOneBitmapIntoAnother(
            &glyph->Bitmap,
            &CharBitmap,
            CharBorder + ShadowOffset,
            CharBorder + ShadowOffset,
            V4(0.0f, 0.0f, 0.0f, 1.0f));
        
    }
    
    RenderOneBitmapIntoAnother(
        &glyph->Bitmap,
        &CharBitmap,
        CharBorder,
        CharBorder,
        V4(1.0f, 1.0f, 1.0f, 1.0f));
    
    DeallocateBitmap(&CharBitmap);
    stbtt_FreeBitmap(Bitmap, 0);
}

tool_font_info LoadFont(char* FilePath, float height, u32 Flags){
    tool_font_info res = {};
    
    stbtt_fontinfo STBFont;
    
    data_buffer TTFBuffer = ReadFileToDataBuffer(FilePath);
    stbtt_InitFont(&STBFont, TTFBuffer.Data, 
                   stbtt_GetFontOffsetForIndex(TTFBuffer.Data, 0));
    
    float Scale = stbtt_ScaleForPixelHeight(&STBFont, height);
    
    int AscenderHeight;
	int DescenderHeight;
	int LineGap;
    
    stbtt_GetFontVMetrics(
        &STBFont,
        &AscenderHeight,
        &DescenderHeight,
        &LineGap);
    
    res.AscenderHeight = (float)AscenderHeight * Scale;
	res.DescenderHeight = (float)DescenderHeight * Scale;
	res.LineGap = (float)LineGap * Scale;
    
    int AtlasWidth = 0;
    int AtlasHeight = 0;
    
    //NOTE(dima): This is for blurring
	int BlurRadius = 2;
	float GaussianBox[256];
	if (Flags & LoadFont_BakeBlur) {
        
		u32 GaussianBoxCompCount = Calcualte2DGaussianBoxComponentsCount(BlurRadius);
		Calculate2DGaussianBox(GaussianBox, BlurRadius);
	}
    
    for(int Codepoint = ' ';
        Codepoint <= '~';
        Codepoint++)
    {
        LoadFontAddCodepoint(
            &res, 
            &STBFont,
            Codepoint,
            &AtlasWidth,
            &AtlasHeight,
            Scale,
            Flags,
            BlurRadius,
            GaussianBox);
    }
    
    if(res.GlyphCount){
        //NOTE(dima): Processing kerning
        res.KerningPairs = (float*)malloc(sizeof(float) * res.GlyphCount * res.GlyphCount);
        
        for (int firstIndex = 0; firstIndex < res.GlyphCount; firstIndex++) {
            for (int secondIndex = 0; secondIndex < res.GlyphCount; secondIndex++) {
                u32 KerningIndex = secondIndex * res.GlyphCount + firstIndex;
                
                int FirstCodepoint = res.Glyphs[firstIndex].Codepoint;
                int SecondCodepoint = res.Glyphs[secondIndex].Codepoint;
                
                int Kern = stbtt_GetGlyphKernAdvance(&STBFont, FirstCodepoint, SecondCodepoint);
                
                res.KerningPairs[KerningIndex] = (float)Kern * Scale;
            }
        }
    }
    
    FreeDataBuffer(&TTFBuffer);
    
    return(res);
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
            
            Group.Count = Src->RegionsCount;
            
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
            u32 TagsByteSize = MAX_TAGS_PER_ASSET * sizeof(asset_tag_header);
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
                    Header->Bitmap.BakeToAtlas = ((Source->BitmapSource.LoadFlags & 
                                                   BitmapLoad_BakeIcon) != 0);
                    
                    //NOTE(dima): Set data size
                    DataByteSize = Asset->Bitmap->Width * Asset->Bitmap->Height * 4;
                }break;
                
                case AssetType_Sound:{
                    b32 SoundAllocatedHere = 0;
                    if(!Source->SoundSource.Sound){
                        Asset->Sound = (sound_info*)malloc(sizeof(sound_info));
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
                        Header->Mesh.VertexTypeSize * Asset->Mesh->VerticesCount + 
                        sizeof(u32) * Asset->Mesh->IndicesCount;
                }break;
            }
            
            //NOTE(dima): Forming header
            Header->LineDataOffset = GetLineOffsetForData();
            Header->Pitch = HeaderByteSize + DataByteSize;
            Header->TagCount = Asset->TagCount;
            Header->AssetType = Asset->Type;
            Header->TotalDataSize = DataByteSize;
            Header->TotalTagsSize = TagsByteSize;
            
            // NOTE(Dima): Forming tags in header
            for (int AssetTagIndex = 0;
                 AssetTagIndex < MAX_TAGS_PER_ASSET;
                 AssetTagIndex++)
            {
                game_asset_tag* From = Asset->Tags + AssetTagIndex;
                asset_tag_header* To = Header->Tags + AssetTagIndex;
                
                To->Type = From->Type;
                To->Value_Float = From->Value_Float;
            }
            
            // NOTE(Dima): Writing asset header
            fwrite(Header, sizeof(asset_header), 1, fp);
            
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

INTERNAL_FUNCTION void WriteFontsChunk(tool_font_info** Fonts,
                                       u32* FontsGroups,
                                       game_asset_tag_hub* FontsTags,
                                       int FontsCount, 
                                       int ChunkIndex)
{
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    // NOTE(Dima): Adding glyphs
    BeginAsset(System, GameAsset_Type_Glyph);
    for (int FontIndex = 0;
         FontIndex < FontsCount;
         FontIndex++)
    {
        tool_font_info* Font = Fonts[FontIndex];
        
        for (int GlyphIndex = 0;
             GlyphIndex < Font->GlyphCount;
             GlyphIndex++)
        {
            added_asset AddedGlyphAsset = AddGlyphAsset(System, &Font->Glyphs[GlyphIndex]);
            
            Font->GlyphIDs[GlyphIndex] = AddedGlyphAsset.ID;
        }
    }
    EndAsset(System);
    
    // NOTE(Dima): Adding fonts
    for (int FontIndex = 0;
         FontIndex < FontsCount;
         FontIndex++)
    {
        tool_font_info* Font = Fonts[FontIndex];
        game_asset_tag_hub* TagHub = FontsTags + FontIndex;
        
        BeginAsset(System, FontsGroups[FontIndex]);
        // NOTE(Dima): Adding font asset
        AddFontAsset(System, Font);
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
    tool_font_info LibMono = LoadFont("../Data/Fonts/LiberationMono-Regular.ttf", 18.0f, LoadFont_BakeShadow);
    tool_font_info Lilita = LoadFont("../Data/Fonts/LilitaOne.ttf", 20.0f, LoadFont_BakeShadow);
    tool_font_info Inconsolata = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 18.0f, LoadFont_BakeBlur);
    tool_font_info PFDIN = LoadFont("../Data/Fonts/PFDinTextCondPro-Regular.ttf", 18.0f, LoadFont_BakeBlur);
    tool_font_info MollyJack = LoadFont("../Data/Fonts/MollyJack.otf", 40.0f, LoadFont_BakeBlur);
    
    tool_font_info* Fonts[] = {
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
        tool_font_info** CurFontsChunk = &Fonts[FontIndex];
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

INTERNAL_FUNCTION void WriteBitmapArray(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    std::string FolderPath("../Data/Images/");
    std::string ListFileName("ToLoadImages.txt");
    
    std::vector<std::string> BitmapPaths;
    
    // NOTE(Dima): Initializing bitmap paths
    Loaded_Strings BitmapNames = LoadStringListFromFile((char*)(FolderPath + ListFileName).c_str());
    int BitmapCount = BitmapNames.Count;
    for(int i = 0; i < BitmapCount; i++){
        BitmapPaths.push_back(FolderPath + std::string(BitmapNames.Strings[i]));
    }
    FreeStringList(&BitmapNames);
    
    // NOTE(Dima): Adding bitmap assets
    BeginAsset(System, GameAsset_Type_Bitmap);
    u32 FirstBitmapID = 0;
    for(int i = 0; i < BitmapPaths.size(); i++){
        added_asset Asset = AddBitmapAsset(System, (char*)BitmapPaths[i].c_str());
        
        if(i == 0){
            FirstBitmapID = Asset.ID;
        }
    }
    EndAsset(System);
    
    // NOTE(Dima): adding bitmap array
    BeginAsset(System, GameAsset_FadeoutBmps);
    AddBitmapArray(System, FirstBitmapID, BitmapCount);
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/BitmapsArray.ja");
}

INTERNAL_FUNCTION void WriteBitmaps(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    
    
    WriteAssetFile(System, "../Data/Bitmaps.ja");
}

INTERNAL_FUNCTION void WriteSounds(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    sound_info Sine = MakeSineSound256(44100 * 2, 44100);
    
    BeginAsset(System, GameAsset_SineTest);
    AddSoundAssetManual(System, &Sine);
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/Sounds.ja");
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
    WriteBitmaps();
    WriteIcons();
    WriteMeshPrimitives();
    WriteFonts();
    WriteBitmapArray();
    WriteSounds();
    
    system("pause");
    return(0);
}