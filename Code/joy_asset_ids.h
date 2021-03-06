#ifndef JOY_ASSET_IDS_H
#define JOY_ASSET_IDS_H

#include "joy_asset_types_shared.h"

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
    AssetType_Skeleton,
    AssetType_AnimationClip,
    AssetType_NodeAnimation,
};

enum asset_state{
    AssetState_Unloaded,
    AssetState_InProgress,
    AssetState_Loaded,
};

typedef u32 asset_id;

#define ASSET_PTR_MEMBER(data_type) data_type* Ptr_##data_type
#define GET_ASSET_PTR_MEMBER(asset, data_type) ((asset)->Data.Ptr_##data_type)

struct asset_material{
    u32 TextureIDs[MaterialChannel_Count];
    
    // NOTE(Dima): These are packed R10 G12 B10
    u32 PackedColors[MaterialChannel_Count];
};

struct asset_node{
    int ParentIndex;
    int FirstChildIndex;
    int ChildCount;
    
    u32 DataOffsetToMeshIndices;
    u32 SizeMeshIndices;
    u32 DataOffsetToFirstMatrix;
    u32 DataOffsetToSecondMatrix;
    
    u32 DataOffsetToName;
    u32 SizeName;
    u32 OneMatrixSize;
    
    int MeshCount;
};

struct asset_model{
    i16 MeshCount;
    i16 MaterialCount;
    i16 NodeCount;
    i16 NodesMeshIndicesStorageCount;
    i16 AnimationCount;
    
    // NOTE(Dima): If ID is 0 then there is no skeleton
    u32 SkeletonID;
    u32 NodesCheckSum;
    
    u32 DataOffsetToMeshIDs;
    u32 DataOffsetToMaterialIDs;
    u32 DataOffsetToNodesSharedDatas;
    u32 DataOffsetToNodesMeshIndicesStorage;
    u32 DataOffsetToAnimationIDs;
    
    u32 SizeMeshIDs;
    u32 SizeMaterialIDs;
    u32 SizeNodesSharedDatas;
    u32 SizeNodesMeshIndicesStorage;
    u32 SizeAnimationIDs;
};

struct asset_skeleton{
    int BoneCount;
    
    u32 DataOffsetToBones;
    u32 SizeBones;
};

struct asset_animation_clip{
    float Duration;
    float TicksPerSecond;
    
    u32 NodesCheckSum;
    
    int NodeAnimationIDsCount;
    
    b32 UsesRootMotion;
    u32 RootMotionNodeAnimID;
    
    u32 DataOffsetToNodeAnimationIDs;
    u32 SizeNodeAnimationIDs;
    
    u32 DataOffsetToName;
    u32 SizeName;
};

struct asset_node_animation{
    int PositionKeysCount;
    int RotationKeysCount;
    int ScalingKeysCount;
    
    u32 DataOffsetToPositionKeysValues;
    u32 DataOffsetToRotationKeysValues;
    u32 DataOffsetToScalingKeysValues;
    
    u32 DataOffsetToPositionKeysTimes;
    u32 DataOffsetToRotationKeysTimes;
    u32 DataOffsetToScalingKeysTimes;
    
    u32 SizePositionKeysValues;
    u32 SizeRotationKeysValues;
    u32 SizeScalingKeysValues;
    
    u32 SizePositionKeysTimes;
    u32 SizeRotationKeysTimes;
    u32 SizeScalingKeysTimes;
    
    u32 NodeIndex;
    b32 IsRootMotion;
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
    mesh_type_context TypeCtx;
    
    int MaterialIndex;
    
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

enum asset_tag_value_type{
    TagValue_Empty,
    TagValue_Float,
    TagValue_Int,
};

struct asset_tag_value {
    union{
        float Value_Float;
        int Value_Int;
    };
    u8 Type;
};

struct asset_tag_header{
    u32 Type;
    
    asset_tag_value Value;
};

inline asset_tag_value TagValue(int Value){
    asset_tag_value Result;
    
    Result.Type = TagValue_Int;
    Result.Value_Int = Value;
    
    return(Result);
}

inline asset_tag_value TagValue(float Value){
    asset_tag_value Result;
    
    Result.Type = TagValue_Float;
    Result.Value_Float = Value;
    
    return(Result);
}

inline asset_tag_header TagHeader(u32 Type, asset_tag_value Value){
    asset_tag_header Result;
    
    Result.Type = Type;
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
    b32 ImmediateLoad;
    
    union{
        asset_model Model;
        asset_skeleton Skeleton;
        asset_material Material;
        asset_bitmap Bitmap;
        asset_array Array;
        asset_glyph Glyph;
        asset_mesh Mesh;
        asset_font Font;
        asset_sound Sound;
        asset_node Node;
        asset_animation_clip AnimationClip;
        asset_node_animation NodeAnim;
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
    NOTE(dima): I store asset entries count here
    because later i will want to make sure that
    the asset file that i load is actual and 
    wont contain new assets group types that we dont't 
    have in our game...
    */
    u32 EntriesCount;
    
    /*
    NOTE(Dima): Groups regions are written to file in one 
    array. Each file group has first region index in that 
    array and region count.
    */
    u32 RegionsCount;
    
    // NOTE(Dima): Not counting zero asset
    u32 EffectiveAssetsCount;
    
    u32 EntriesByteOffset;
    u32 EntriesRegionsByteOffset;
    u32 LinesOffsetsByteOffset;
    u32 AssetLinesByteOffset;
};

enum asset_tag_font_type_value{
    TagFont_Regular,
    TagFont_Bold,
};

enum asset_tag_character_type{
    TagCharacter_Bear,
    TagCharacter_Fox,
    TagCharacter_Rabbit,
    TagCharacter_Moose,
    TagCharacter_Deer,
    TagCharacter_Coyote,
    
    TagCharacter_Bull,
    TagCharacter_Chicken,
    TagCharacter_Cow,
    TagCharacter_Horse,
    TagCharacter_Pig,
    //TagCharacter_Sheep,
    
    TagCharacter_Caterpillar,
    TagCharacter_Hornet,
    TagCharacter_LadyBug,
    TagCharacter_Mantis,
    TagCharacter_Moth,
    TagCharacter_Rhinoceros,
};

enum asset_tag_idle_anim{
    TagIdleAnim_Idle0,
    TagIdleAnim_Idle1,
    TagIdleAnim_Idle2,
    TagIdleAnim_Idle3,
    TagIdleAnim_Idle4,
    TagIdleAnim_Idle5,
};

enum asset_tag_attack_anim{
    TagAttack_0,
    TagAttack_1,
    TagAttack_2,
    TagAttack_3,
    TagAttack_4,
    TagAttack_5,
};

enum asset_tag_damage{
    TagDamage_Low,
    TagDamage_Medium,
    TagDamage_Heavy,
};

enum asset_tag_direction{
    TagDirection_Forward,
    TagDirection_Left,
    TagDirection_Back,
    TagDirection_Right,
};

enum asset_tag_type{
    AssetTag_FontType,
    AssetTag_LOD,
    AssetTag_Size,
    
    AssetTag_Character,
    AssetTag_IdleAnim,
    
    AssetTag_Count,
};

struct asset_tag{
    u32 Type;
    
    union{
        float Value_Float;
        int Value_Int;
    };
};

enum asset_entries
{
    // NOTE(Dima): Icons for GUI
    AssetEntry_CheckboxMark,
    AssetEntry_ChamomileIcon,
    
    // NOTE(Dima): Generated sounds
    Asset_SineTest,
    
    // NOTE(Dima): Mesh primitives
    AssetEntry_Cube,
    AssetEntry_Plane,
    AssetEntry_Sphere,
    AssetEntry_Cylynder,
    AssetEntry_UtahTeapot,
    
    AssetEntry_FadeoutBmps,
    
    AssetEntry_Model_Character,
    AssetEntry_Anim_Failure,
    AssetEntry_Anim_Fall,
    AssetEntry_Anim_Idle,
    AssetEntry_Anim_JumpUp,
    AssetEntry_Anim_Land,
    AssetEntry_Anim_Roll,
    AssetEntry_Anim_Run,
    AssetEntry_Anim_Sleep,
    AssetEntry_Anim_Success,
    AssetEntry_Anim_Talk,
    AssetEntry_Anim_Walk,
    AssetEntry_Anim_Die,
    AssetEntry_Anim_Attack,
    AssetEntry_Anim_TakeDamage,
    AssetEntry_Anim_Throw,
    AssetEntry_Model_TempForCounting,
    
    // NOTE(Dima): Animations
    AssetEntry_Man,
    
    // NOTE(Dima): Typed assets
    AssetEntry_Type_Bitmap,
    AssetEntry_Type_Font,
    AssetEntry_Type_Glyph,
    AssetEntry_Type_Mesh,
    AssetEntry_Type_Material,
    AssetEntry_Type_BitmapArray,
    AssetEntry_Type_Skeleton,
    AssetEntry_Type_AnimationClip,
    AssetEntry_Type_NodeAnim,
    
    // NOTE(Dima): Fonts
    AssetEntry_LiberationMono,
    AssetEntry_LilitaOne,
    AssetEntry_Inconsolata,
    AssetEntry_PFDIN,
    AssetEntry_MollyJackFont,
    
    AssetEntry_Count,
};

#endif