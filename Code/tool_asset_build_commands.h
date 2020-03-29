#ifndef TOOL_ASSET_BUILD_COMMANDS
#define TOOL_ASSET_BUILD_COMMANDS

#include "tool_asset_build_types.h"
#include "tool_asset_build_loading.h"

enum{
    Immediate_No,
    Immediate_Yes,
};

struct added_asset {
	game_asset* Asset;
	game_asset_source* Source;
	game_asset_freearea* Freearea;
	asset_header* FileHeader;
    u32 ID;
};

void BeginAsset(asset_system* System, u32 GroupID);
void EndAsset(asset_system* System);

game_asset_tag* AddTag(asset_system* System, u32 TagType);
void AddFloatTag(asset_system* System, u32 TagType, float TagValue);
void AddIntTag(asset_system* System, u32 TagType, int TagValue);
void AddEmptyTag(asset_system* System, u32 TagType);
void AddTagHubToAsset(asset_system* System, game_asset_tag_hub* TagHub);

added_asset AddBitmapAsset(asset_system* System, 
                           char* Path, 
                           u32 BitmapLoadFlags = 0);

added_asset AddBitmapAssetManual(asset_system* System, 
                                 tool_bmp_info* Bitmap, 
                                 u32 BitmapLoadFlags = 0);

added_asset AddIconAsset(asset_system* System,
                         char* Path);

added_asset AddSoundAsset(asset_system* System, 
                          char* Path);

added_asset AddSoundAssetManual(asset_system* System, 
                                tool_sound_info* Sound);

added_asset AddMeshAsset(asset_system* System, 
                         tool_mesh_info* Mesh);

added_asset AddSkeletonAsset(asset_system* System,
                             tool_skeleton_info* Skeleton);

added_asset AddNodeAsset(asset_system* System, tool_node_info* Node);

added_asset AddModelAsset(asset_system* System,
                          tool_model_info* Model);

added_asset AddMaterialAsset(asset_system* System, 
                             tool_material_info* Material);

added_asset AddFontAsset(asset_system* System,
                         tool_font_info* FontInfo);

added_asset AddGlyphAssetInternal(asset_system* System,
                                  tool_glyph_info* GlyphInfo, 
                                  u32 BitmapID);

added_asset AddGlyphAsset(asset_system* System,
                          tool_glyph_info* Glyph);

added_asset AddArrayAsset(asset_system* System, 
                          u32 FirstID, 
                          int Count);


void InitAssetFile(asset_system* Assets);
void WriteAssetFile(asset_system* Assets, char* FileName);

#endif