#ifndef TOOL_ASSET_BUILD_TYPES
#define TOOL_ASSET_BUILD_TYPES

#include "joy_asset_ids.h"
#include "joy_math.h"
#include "joy_strings.h"

#include <vector>
#include <unordered_map>

#define Assert(cond) if(!(cond)){ *((int*)0) = 0;}
#define ASSERT(cond) if(!(cond)){ *((int*)0) = 0;}

#define ArrayCount(arr) (sizeof(arr) / sizeof((arr)[0]))
#define InvalidCodePath Assert(!"Invalid code path!");

#define INVALID_CODE_PATH Assert(!"Invalid code path!");
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

#define Kilobytes(count) ((count) * 1000)
#define Megabytes(count) ((count) * 1000000)
#define Gigabytes(count) ((count) * 1000000000)

#define Kibibytes(count) ((count) * 1024)
#define Mibibytes(count) ((count) * 1024 * 1024)
#define Gibibytes(count) ((count) * 1024 * 1024 * 1024)

#define GlobalVariable static
#define InternalFunction static
#define LocalAsGlobal static

#define GLOBAL_VARIABLE static
#define INTERNAL_FUNCTION static
#define LOCAL_AS_GLOBAL static

#ifndef Min
#define Min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef Max
#define Max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef Abs
#define Abs(a) ((a) >= 0) ? (a) : -(a)
#endif

#define JOY_TRUE 1
#define JOY_FALSE 0

struct Loaded_Strings{
    char** Strings;
    int Count;
};

struct data_buffer {
	u8* Data;
	u64 Size;
};


struct tool_bmp_info{
    void* Pixels;
    int Width;
    int Height;
    int Pitch;
    float WidthOverHeight;
};

struct tool_sound_info{
    // NOTE(Dima): Left and Right channels
    i16* Samples[2];
    
    int Channels;
    int SampleCount;
    int SamplesPerSec;
};

struct tool_glyph_info{
    int Codepoint;
    
    int Width;
    int Height;
    
    tool_bmp_info Bitmap;
    
    /*Theese are offset from glyph origin to top-left of bitmap*/
	float XOffset;
	float YOffset;
	float Advance;
	float LeftBearingX;
};

#define FONT_INFO_MAX_GLYPH_COUNT 256
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


struct vertex_weight{
    int BoneID;
    float Weight;
};

struct vertex_weights{
    std::vector<vertex_weight> Weights;
    
    float SumOfSquaredWeights;
};

struct tool_mesh_info{
    u32* Indices;
    int IndicesCount;
    
    void* Vertices;
    int VerticesCount;
    
    mesh_type_context TypeCtx;
};

struct tool_node_info{
    // NOTE(Dima): Theese are indcies in array of meshes
    std::vector<int> MeshIndices;
    
    // NOTE(Dima): These are stored mesh IDs
    std::vector<int> MeshIDs;
    
    node_shared_data Shared;
};

struct tool_model_info{
    std::vector<u32> MeshIDs;
    std::vector<u32> MaterialIDs;
    std::vector<u32> AnimationIDs;
    
    u32 SkeletonID;
    
    int MeshCount;
    int MaterialCount;
    int AnimationCount;
    
    std::vector<tool_node_info> Nodes;
    
    std::vector<node_shared_data> NodesSharedDatas;
    std::vector<u32> NodeMeshIndicesStorage;
};

struct tool_node_animation{
    std::string NodeName;
    
    int NodeIndex;
    
    std::vector<v3> PositionValues;
    std::vector<quat> RotationValues;
    std::vector<v3> ScalingValues;
    
    std::vector<float> PositionTimes;
    std::vector<float> RotationTimes;
    std::vector<float> ScalingTimes;
};

struct tool_animation_info{
    float Duration;
    float TicksPerSecond;
    
    std::string Name;
    
    char StoreName[1024];
    
    std::vector<tool_node_animation> NodeAnimations;
    std::vector<u32> NodeAnimationsStoredIDs;
};

struct tool_skeleton_info{
    u32 CheckSum;
    
    std::vector<bone_info> Bones;
};

struct tool_material_info{
    u32 BitmapArrayIDs[MaterialTexture_Count];
    
    u32 ColorDiffusePacked;
    u32 ColorAmbientPacked;
    u32 ColorSpecularPacked;
    u32 ColorEmissivePacked;
};

struct game_asset_group_region {
    u32 FirstAssetIndex;
    u32 AssetCount;
};

struct game_asset_group{
    std::vector<game_asset_group_region> Regions;
};

enum game_asset_tag_value_type{
    GameAssetTagValue_Empty,
    GameAssetTagValue_Float,
    GameAssetTagValue_Int,
};

struct game_asset_tag {
    u32 Type;
    u32 ValueType;
    
    u32 InTagArrayIndex;
    
    union {
        float Value_Float;
        int Value_Int;
    };
};

struct game_asset_tag_hub{
    public:
    std::vector<game_asset_tag> Tags;
    std::vector<u32> TagValueTypes;
    
    private:
    game_asset_tag* FindTag(u32 TagType){
        game_asset_tag* Result = 0;
        
        for (int TagIndex = 0;
             TagIndex < Tags.size();
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
    
    game_asset_tag* AddTag(u32 TagType, u32 ValueType){
        game_asset_tag* Result = FindTag(TagType);
        
        game_asset_tag NewTag;
        
        NewTag.Type = TagType;
        NewTag.InTagArrayIndex = Tags.size();
        NewTag.ValueType = ValueType;
        
        Tags.push_back(NewTag);
        TagValueTypes.push_back({});
        
        Result = &Tags[Tags.size() - 1];
        
        return(Result);
    }
    
    public:
    static game_asset_tag_hub Empty(){
        game_asset_tag_hub Result = {};
        
        return(Result);
    }
    
    game_asset_tag_hub& AddIntTag(u32 TagType, int Value){
        game_asset_tag* Tag = AddTag(TagType, GameAssetTagValue_Int);
        
        if(Tag){
            TagValueTypes[Tag->InTagArrayIndex] = GameAssetTagValue_Int;
            Tag->Value_Int = Value;
        }
        
        return(*this);
    }
    
    game_asset_tag_hub& AddFloatTag(u32 TagType, float Value){
        game_asset_tag* Tag = AddTag(TagType, GameAssetTagValue_Float);
        
        if(Tag){
            TagValueTypes[Tag->InTagArrayIndex] = GameAssetTagValue_Float;
            Tag->Value_Float = Value;
        }
        
        return(*this);
    }
    
    game_asset_tag_hub& AddEmptyTag(u32 TagType){
        game_asset_tag* Tag = AddTag(TagType, GameAssetTagValue_Empty);
        
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
    
    std::vector<game_asset_tag> Tags;
    
    union {
        tool_bmp_info* Bitmap;
        tool_font_info* Font;
        tool_sound_info* Sound;
        tool_mesh_info* Mesh;
        tool_glyph_info* Glyph;
        tool_model_info* Model;
        tool_material_info* Material;
        tool_skeleton_info* Skeleton;
        tool_node_info* Node;
        tool_animation_info* Animation;
        tool_node_animation* NodeAnim;
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
    tool_bmp_info* BitmapInfo;
    u32 LoadFlags;
};

struct game_asset_source_mesh {
    tool_mesh_info* MeshInfo;
};

struct game_asset_source_model {
    tool_model_info* ModelInfo;
};

struct game_asset_source_node{
    tool_node_info* NodeInfo;
};

struct game_asset_source_animation_clip{
    tool_animation_info* AnimationInfo;
};

struct game_asset_source_node_anim{
    tool_node_animation* NodeAnimInfo;
};

struct game_asset_source_skeleton{
    tool_skeleton_info* SkeletonInfo;
};

struct game_asset_source_material{
    tool_material_info* MaterialInfo;
};

struct game_asset_source_sound {
    char* Path;
    tool_sound_info* Sound;
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
        game_asset_source_skeleton SkeletonSource;
        game_asset_source_node NodeSource;
        game_asset_source_animation_clip AnimationSource;
        game_asset_source_node_anim NodeAnimSource;
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


#endif