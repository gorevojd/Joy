#ifndef JOY_ASSETS_H
#define JOY_ASSETS_H

#include "joy_types.h"
#include "joy_defines.h"
#include "joy_asset_types.h"
#include "joy_asset_util.h"
#include "joy_platform.h"
#include "joy_memory.h"

struct Asset_Atlas{
    Bmp_Info Bitmap;
    int AtX;
    int AtY;
    int Dim;
    int MaxInRowHeight;
    
    float OneOverDim;
};

struct asset{
    union{
        
    };
};

// NOTE(dima): Bitmaps are stored in gamma-corrected premultiplied alpha format
struct Assets{
    Memory_Region* Region;
    
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
    
    Bmp_Info checkboxMark;
    
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