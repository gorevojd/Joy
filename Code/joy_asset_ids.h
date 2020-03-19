#ifndef JOY_ASSET_IDS_H
#define JOY_ASSET_IDS_H

#include "joy_types.h"

enum asset_type{
    AssetType_None,
    
    // NOTE(Dima): These are actual types
    AssetType_Bitmap,
    AssetType_BitmapArray,
    AssetType_Mesh,
    AssetType_Sound,
    AssetType_Font,
    AssetType_Glyph,
    
    // NOTE(Dima) These are fake types for fast access through macros
    AssetType_Type_bmp_info = AssetType_Bitmap,
    AssetType_Type_bmp_array_info = AssetType_BitmapArray,
    AssetType_Type_mesh_info = AssetType_Mesh,
    AssetType_Type_sound_info = AssetType_Sound,
    AssetType_Type_font_info = AssetType_Font,
    AssetType_Type_glyph_info = AssetType_Glyph,
};

enum asset_state{
    AssetState_Unloaded,
    AssetState_InProgress,
    AssetState_Loaded,
};

typedef u32 asset_id;

#define ASSET_VALUE_MEMBER(data_type) data_type Data_##data_type
#define ASSET_PTR_MEMBER(data_type) data_type* Ptr_##data_type
#define GET_ASSET_VALUE_MEMBER(asset, data_type) ((asset)->Data_##data_type)
#define GET_ASSET_PTR_MEMBER(asset, data_type) ((asset)->Ptr_##data_type)


struct asset_material{
    asset_id Diffuse;
    asset_id Specular;
    asset_id Normal;
    asset_id Emission;
    
    asset_id Albedo;
    asset_id Roughness;
    asset_id Metallic;
};

struct asset_model{
    asset_id MeshID;
    asset_id MaterialID;
};

struct asset_bitmap{
    u32 Width;
    u32 Height;
    
    b32 BakeToAtlas;
};

struct asset_bitmap_array{
    u32 FirstBmpID;
    int Count;
};

struct asset_glyph{
    asset_id BitmapID;
    
    int Codepoint;
    
    u32 BitmapWidth;
    u32 BitmapHeight;
    float BitmapWidthOverHeight;
    
    float XOffset;
	float YOffset;
	float Advance;
	float LeftBearingX;
};

struct asset_font{
    float AscenderHeight;
	float DescenderHeight;
	float LineGap;
    
    int GlyphCount;
    
    u32 DataOffsetToMapping;
    u32 DataOffsetToKerning;
    u32 DataOffsetToIDs;
    
    u32 MappingSize;
    u32 KerningSize;
    u32 IDsSize;
};

struct asset_mesh{
    u32 MeshType;
    u32 VertexTypeSize;
    
    u32 VerticesCount;
    u32 IndicesCount;
    
    u32 DataVerticesSize;
    u32 DataIndicesSize;
    
    u32 DataOffsetToVertices;
    u32 DataOffsetToIndices;
};

struct asset_sound{
    int SampleCount;
    int SamplesPerSec;
    int Channels;
};

#define MAX_TAGS_PER_ASSET 4

struct asset_tag_header{
    u32 Type;
    
    union {
        float Value_Float;
        int Value_Int;
    };
};

struct asset_header{
    asset_tag_header Tags[MAX_TAGS_PER_ASSET];
    u32 TagCount;
    
    u32 AssetType;
    
    u32 LineDataOffset;
    
    u32 TotalDataSize;
    u32 TotalTagsSize;
    
    u32 Pitch;
    
    union{
        asset_model Model;
        asset_material Material;
        asset_bitmap Bitmap;
        asset_bitmap_array BmpArray;
        asset_glyph Glyph;
        asset_mesh Mesh;
        asset_font Font;
        asset_sound Sound;
    };
};

#define ASSET_GROUP_REGIONS_COUNT 8
#define ASSET_FILE_VERSION_MAJOR 1
#define ASSET_FILE_VERSION_MINOR 0

inline u32 GetVersionInt(int Major, int Minor){
    u32 Result = ((Major & 0xFFFF) << 16) | (Minor & 0xFFFF);
    
    return(Result);
}

inline u32 GetLineOffsetForData(){
    return(sizeof(asset_header));
}

struct asset_file_group_region{
    u32 FirstAssetIndex;
    u32 AssetCount;
};

struct asset_file_group{
    asset_file_group_region Regions[ASSET_GROUP_REGIONS_COUNT];
    int Count;
};

struct asset_file_header{
    u8 FileHeader[4];
    
    u32 Version;
    
    /*
    NOTE(dima): I store asset groups count here
    because later i will want to make sure that
    the asset file that i load is actual and 
    wont contain new assets group types that we dont't 
    have in our game...
    */
    u32 GroupsCount;
    
    // NOTE(Dima): Not counting zero asset
    u32 EffectiveAssetsCount;
    
    u32 GroupsByteOffset;
    u32 LinesOffsetsByteOffset;
    u32 AssetLinesByteOffset;
};

enum asset_tag_font_type_value{
    AssetFontTypeTag_Regular,
    AssetFontTypeTag_Bold,
};

enum asset_tag_type{
    AssetTag_FontType,
    AssetTag_LOD,
    AssetTag_Counter,
};

struct asset_tag{
    u32 Type;
    
    union{
        float Value_Float;
        int Value_Int;
    };
};

enum asset_group_type{
    GameAsset_FadeoutBmps,
    
    GameAsset_CheckboxMark,
    GameAsset_ChamomileIcon,
    
    GameAsset_SineTest,
    
    GameAsset_Cube,
    GameAsset_Plane,
    GameAsset_Sphere,
    GameAsset_Cylynder,
    
    GameAsset_LiberationMono,
    GameAsset_LilitaOne,
    GameAsset_Inconsolata,
    GameAsset_PFDIN,
    GameAsset_MollyJackFont,
    
    GameAsset_Type_Bitmap,
    GameAsset_Type_Font,
    GameAsset_Type_Glyph,
    
    GameAsset_Count,
};

#endif