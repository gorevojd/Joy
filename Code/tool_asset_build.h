#ifndef TOOL_ASSET_BUILD_H
#define TOOL_ASSET_BUILD_H

#include "joy_defines.h"
#include "joy_types.h"
#include "joy_asset_types.h"
#include "joy_asset_ids.h"

#include <vector>


struct Loaded_Strings{
    char** Strings;
    int Count;
};

struct data_buffer {
	u8* Data;
	u64 Size;
};

struct tool_glyph_info{
    int Codepoint;
    
    int Width;
    int Height;
    
    bmp_info Bitmap;
    
    /*Theese are offset from glyph origin to top-left of bitmap*/
	float XOffset;
	float YOffset;
	float Advance;
	float LeftBearingX;
};

struct tool_font_info{
    float AscenderHeight;
	float DescenderHeight;
	float LineGap;
    
    float* KerningPairs;
    
    u32 GlyphIDs[FONT_INFO_MAX_GLYPH_COUNT];
    
    tool_glyph_info Glyphs[FONT_INFO_MAX_GLYPH_COUNT];
    int GlyphCount;
    
    int Codepoint2Glyph[FONT_INFO_MAX_GLYPH_COUNT];
};

struct game_asset_group_region {
	u32 FirstAssetIndex;
	u32 AssetCount;
};

struct game_asset_group{
    game_asset_group_region Regions[ASSET_GROUP_REGIONS_COUNT];
    int RegionsCount;
};

enum game_asset_tag_value_type{
    GameAssetTagValue_Float,
    GameAssetTagValue_Int,
    GameAssetTagValue_Empty,
};

struct game_asset_tag {
	u32 Type;
    
    u32 InTagArrayIndex;
    
	union {
		float Value_Float;
		int Value_Int;
	};
};

struct game_asset_tag_hub{
    public:
    game_asset_tag Tags[MAX_TAGS_PER_ASSET];
	u32 TagValueTypes[MAX_TAGS_PER_ASSET];
    int TagCount;
    
    private:
    game_asset_tag* FindTag(u32 TagType){
        game_asset_tag* Result = 0;
        
        for (int TagIndex = 0;
             TagIndex < TagCount;
             TagIndex++)
        {
            game_asset_tag* Tag = &Tags[TagIndex];
            if (Tag->Type == TagType) {
                Result = Tag;
                break;
            }
        }
        
        return(Result);
    }
    
    game_asset_tag* AddTag(u32 TagType){
        game_asset_tag* Result = FindTag(TagType);
        
        if(!Result && (TagCount < MAX_TAGS_PER_ASSET - 1)){
            int ResultIndex = TagCount++;
            Result = &Tags[ResultIndex];
            Result->Type = TagType;
            Result->InTagArrayIndex = ResultIndex;
        }
        
        return(Result);
    }
    
    public:
    static game_asset_tag_hub Empty(){
        game_asset_tag_hub Result = {};
        
        return(Result);
    }
    
    game_asset_tag_hub& AddIntTag(u32 TagType, int Value){
        game_asset_tag* Tag = AddTag(TagType);
        
        if(Tag){
            TagValueTypes[Tag->InTagArrayIndex] = GameAssetTagValue_Int;
            Tag->Value_Int = Value;
        }
        
        return(*this);
    }
    
    game_asset_tag_hub& AddFloatTag(u32 TagType, float Value){
        game_asset_tag* Tag = AddTag(TagType);
        
        if(Tag){
            TagValueTypes[Tag->InTagArrayIndex] = GameAssetTagValue_Float;
            Tag->Value_Float = Value;
        }
        
        return(*this);
    }
    
    game_asset_tag_hub& AddEmptyTag(u32 TagType){
        game_asset_tag* Tag = AddTag(TagType);
        
        if(Tag){
            TagValueTypes[Tag->InTagArrayIndex] = GameAssetTagValue_Empty;
            Tag->Value_Int = 1;
        }
        
        return(*this);
    }
};

struct game_asset {
	u32 ID;
    
	u32 Type;
    
	game_asset_tag Tags[MAX_TAGS_PER_ASSET];
	int TagCount;
    
	union {
		bmp_info* Bitmap;
		tool_font_info* Font;
		sound_info* Sound;
		model_info* Model;
		mesh_info* Mesh;
		tool_glyph_info* Glyph;
	};
};

/*
 NOTE(dima): Asset sources
*/

enum bitmap_asset_load_flags{
    BitmapLoad_BakeIcon = 1,
};

struct game_asset_source_bitmap {
	char* Path;
    bmp_info* BitmapInfo;
    u32 LoadFlags;
};

struct game_asset_source_mesh {
	mesh_info* MeshInfo;
};

struct game_asset_source_model {
    // NOTE(Dima): Empty
};

struct game_asset_source_material{
    // NOTE(Dima): Empty
};

struct game_asset_source_sound {
	char* Path;
    sound_info* Sound;
};

struct game_asset_source_font {
	tool_font_info* FontInfo;
};

struct game_asset_source_glyph {
	tool_glyph_info* Glyph;
};

struct game_asset_source {
	union {
		game_asset_source_bitmap BitmapSource;
		game_asset_source_sound SoundSource;
		game_asset_source_font FontSource;
		game_asset_source_glyph GlyphSource;
		game_asset_source_mesh MeshSource;
        game_asset_source_model ModelSource;
        game_asset_source_material MaterialSource;
    };
};

//NOTE(dima): Assets freeareas
#define FREEAREA_SLOTS_COUNT 4
struct game_asset_freearea {
	void* Pointers[FREEAREA_SLOTS_COUNT];
	int SetCount;
};


//NOTE(dima): Asset system
#define TEMP_STORED_ASSET_COUNT 2048
struct asset_system {
	u32 AssetTypes[TEMP_STORED_ASSET_COUNT];
	game_asset Assets[TEMP_STORED_ASSET_COUNT];
	game_asset_source AssetSources[TEMP_STORED_ASSET_COUNT];
	game_asset_freearea AssetFreeareas[TEMP_STORED_ASSET_COUNT];
	asset_header FileHeaders[TEMP_STORED_ASSET_COUNT];
    
	game_asset_group AssetGroups[GameAsset_Count];
    
	u32 AssetCount;
	game_asset_group* CurrentGroup;
	game_asset* PrevAssetPointer;
};


mesh_info MakeMesh(
std::vector<v3>& Positions,
std::vector<v2>& TexCoords,
std::vector<v3>& Normals,
std::vector<v3>& Tangents,
std::vector<v3>& Colors,
std::vector<u32> Indices,
b32 CalculateNormals,
b32 CalculateTangents);
mesh_info MakePlane();
mesh_info MakeCube();
mesh_info MakeSphere(int Segments, int Rings);
mesh_info MakeCylynder(float Height, float Radius, int SidesCount) ;

sound_info MakeSound(const std::vector<i16>& Samples,
                     int SamplesPerSec);
sound_info MakeSineSound(const std::vector<int>& Frequencies, 
                         int SampleCount,
                         int SamplesPerSec);
sound_info MakeSineSound256(int SampleCount, int SamplesPerSec);

#endif