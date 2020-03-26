#ifndef JOY_ASSET_IDS_H
#define JOY_ASSET_IDS_H

#include "joy_types.h"

enum asset_type{
    AssetType_None,
    
    // NOTE(Dima): These are actual types
    AssetType_Bitmap,
    AssetType_Array,
    AssetType_Mesh,
    AssetType_Sound,
    AssetType_Font,
    AssetType_Glyph,
    AssetType_Model,
    AssetType_Material,
};

enum asset_state{
    AssetState_Unloaded,
    AssetState_InProgress,
    AssetState_Loaded,
};

typedef u32 asset_id;

#define ASSET_VALUE_MEMBER(data_type) data_type Data_##data_type
#define GET_ASSET_VALUE_MEMBER(asset, data_type) ((asset)->Data_##data_type)
#define ASSET_PTR_MEMBER(data_type) data_type* Ptr_##data_type
#define GET_ASSET_PTR_MEMBER(asset, data_type) ((asset)->Ptr_##data_type)

enum material_texture_type{
    MaterialTexture_Diffuse,
    MaterialTexture_Specular,
    MaterialTexture_Ambient,
    MaterialTexture_Emissive,
    MaterialTexture_Height,
    MaterialTexture_Normals,
    MaterialTexture_Shininess,
    MaterialTexture_Opacity,
    MaterialTexture_Displacement,
    MaterialTexture_Lightmap,
    MaterialTexture_Reflection,
    MaterialTexture_Unknown,
    
    MaterialTexture_Count,
};

struct asset_material{
    u32 BitmapArrayIDs[MaterialTexture_Count];
};

struct asset_model{
    int MeshCount;
    int MaterialCount;
    
    u32 DataOffsetToMeshIDs;
    u32 DataOffsetToMaterialIDs;
    
    u32 SizeMeshIDs;
    u32 SizeMaterialIDs;
};

struct asset_bitmap{
    u32 Width;
    u32 Height;
    
    b32 BakeToAtlas;
};

struct asset_array{
    u32 FirstID;
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
    
    u32 DataOffsetToLeftChannel;
    u32 DataOffsetToRightChannel;
};

union asset_tag_value {
    float Value_Float;
    int Value_Int;
};

enum asset_tag_value_type{
    AssetTagValue_Empty,
    AssetTagValue_Float,
    AssetTagValue_Int,
};

struct asset_tag_header{
    u32 Type;
    
    u32 ValueType;
    
    asset_tag_value Value;
};

inline asset_tag_header TagHeader(u32 Type, u32 ValueType, asset_tag_value Value){
    asset_tag_header Result;
    
    Result.Type = Type;
    Result.ValueType = ValueType;
    Result.Value = Value;
    
    return(Result);
}

struct asset_header{
    u32 TagCount;
    
    u32 AssetType;
    
    u32 LineTagOffset;
    u32 LineDataOffset;
    
    u32 TotalDataSize;
    u32 TotalTagsSize;
    
    u32 Pitch;
    
    union{
        asset_model Model;
        asset_material Material;
        asset_bitmap Bitmap;
        asset_array Array;
        asset_glyph Glyph;
        asset_mesh Mesh;
        asset_font Font;
        asset_sound Sound;
    };
};

//#define ASSET_GROUP_REGIONS_COUNT 8
#define ASSET_FILE_VERSION_MAJOR 1
#define ASSET_FILE_VERSION_MINOR 0

inline u32 GetVersionInt(int Major, int Minor){
    u32 Result = ((Major & 0xFFFF) << 16) | (Minor & 0xFFFF);
    
    return(Result);
}

struct asset_file_group_region{
    u32 FirstAssetIndex;
    u32 AssetCount;
};

struct asset_file_group{
    int FirstRegionIndex;
    int RegionCount;
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
    
    /*
    NOTE(Dima): Groups regions are written to file in one 
    array. Each file group has first region index in that 
    array and region count.
    */
    u32 RegionsCount;
    
    // NOTE(Dima): Not counting zero asset
    u32 EffectiveAssetsCount;
    
    u32 GroupsByteOffset;
    u32 GroupsRegionsByteOffset;
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
    AssetTag_Size,
};

struct asset_tag{
    u32 Type;
    
    union{
        float Value_Float;
        int Value_Int;
    };
};

enum asset_group_type{
    // NOTE(Dima): Bitmap array
    GameAsset_FadeoutBmps,
    
    // NOTE(Dima): Icons for GUI
    GameAsset_CheckboxMark,
    GameAsset_ChamomileIcon,
    
    // NOTE(Dima): Generated sounds
    GameAsset_SineTest,
    
    // NOTE(Dima): Mesh primitives
    GameAsset_Cube,
    GameAsset_Plane,
    GameAsset_Sphere,
    GameAsset_Cylynder,
    
    // NOTE(Dima): Meshes
    GameAsset_Bathroom,
    GameAsset_Heart,
    GameAsset_KindPlane,
    GameAsset_Podkova,
    GameAsset_RubbishBin,
    GameAsset_Snowman,
    GameAsset_Stool,
    GameAsset_Toilet,
    GameAsset_Vase,
    
    // NOTE(Dima): Fonts
    GameAsset_LiberationMono,
    GameAsset_LilitaOne,
    GameAsset_Inconsolata,
    GameAsset_PFDIN,
    GameAsset_MollyJackFont,
    
    // NOTE(Dima): Typed assets
    GameAsset_Type_Bitmap,
    GameAsset_Type_Font,
    GameAsset_Type_Glyph,
    GameAsset_Type_Mesh,
    GameAsset_Type_Material,
    GameAsset_Type_BitmapArray,
    
    GameAsset_Count,
};

#endif