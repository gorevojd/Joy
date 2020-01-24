#ifndef JOY_ASSETS_H
#define JOY_ASSETS_H

#include "joy_types.h"
#include "joy_defines.h"
#include "joy_asset_types.h"
#include "joy_asset_util.h"
#include "joy_platform.h"
#include "joy_memory.h"

struct asset{
    Memory_Entry* DataMemoryEntry;
    
    union{
        
    };
};

// NOTE(dima): Bitmaps are stored in gamma-corrected premultiplied alpha format
struct Assets{
    Memory_Region* Region;
    
    // NOTE(Dima): Memory entries
    Memory_Box MemBox;
    
    // NOTE(Dima): Assets
    Asset_Atlas MainLargeAtlas;
    
    Bmp_Info sunset;
    Bmp_Info sunsetOrange;
    Bmp_Info sunsetField;
    Bmp_Info sunsetMountains;
    Bmp_Info sunsetPurple;
    Bmp_Info sunrise;
    Bmp_Info mountainsFuji;
    Bmp_Info roadClouds;
    
    Bmp_Info* fadeoutBmps;
    int fadeoutBmpsCount;
    
    Bmp_Info CheckboxMark;
    Bmp_Info Folder;
    Bmp_Info ClosePng;
    Bmp_Info PlayPng;
    Bmp_Info PlusPng;
    Bmp_Info StopPng;
    Bmp_Info PowerPng;
    
    Sound_Info SineTest1;
    Sound_Info SineTest2;
    
    Mesh_Info cube;
    Mesh_Info plane;
    Mesh_Info sphere;
    Mesh_Info cylynder;
    
    Font_Info liberationMono;
    Font_Info lilitaOne;
    Font_Info inconsolataBold;
    Font_Info pfdin;
};

void InitAssets(Assets* assets, Memory_Region* region);

#endif