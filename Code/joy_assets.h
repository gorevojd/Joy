#ifndef JOY_ASSETS_H
#define JOY_ASSETS_H

#include "joy_types.h"
#include "joy_defines.h"
#include "joy_asset_types.h"
#include "joy_asset_util.h"
#include "joy_platform.h"
#include "joy_memory.h"

enum asset_type{
    AssetType_Bitmap,
    AssetType_BitmapArray,
    AssetType_Mesh,
    AssetType_Sound,
    AssetType_Font,
    AssetType_Glyph,
};

struct asset{
    mem_entry* DataMemoryEntry;
    
    u32 Type;
    
    asset* Next;
    asset* Prev;
    
    union{
        font_info Font;
        bmp_info Bitmap;
        bmp_array_info BmpArray;
        mesh_info Mesh;
        glyph_info Glyph;
        sound_info Sound;
    };
};

enum asset_family{
    AssetFamily_FadeoutBmps,
    
    AssetFamily_CheckboxMark,
    AssetFamily_ChamomileIcon,
    
    AssetFamily_SineTest1,
    AssetFamily_SineTest2,
    
    AssetFamily_Cube,
    AssetFamily_Plane,
    AssetFamily_Sphere,
    AssetFamily_Cylynder,
    
    AssetFamily_LiberationMono,
    AssetFamily_LilitaOne,
    AssetFamily_Inconsolata,
    AssetFamily_PFDIN,
    AssetFamily_MollyJackFont,
};

// NOTE(dima): Bitmaps are stored in gamma-corrected premultiplied alpha format
struct assets{
    mem_region* Region;
    
    // NOTE(Dima): Memory entries
    mem_box MemBox;
    
    // NOTE(Dima): Assets
    Asset_Atlas MainLargeAtlas;
    
    asset SentinelAsset;
    
    bmp_info* fadeoutBmps;
    int fadeoutBmpsCount;
    
    bmp_info CheckboxMark;
    bmp_info ChamomileIcon;
    
    sound_info SineTest1;
    sound_info SineTest2;
    
    mesh_info Cube;
    mesh_info Plane;
    mesh_info Sphere;
    mesh_info Cylynder;
    
    font_info liberationMono;
    font_info lilitaOne;
    font_info inconsolataBold;
    font_info pfdin;
    font_info MollyJackFont;
};

void InitAssets(assets* Assets);

#endif