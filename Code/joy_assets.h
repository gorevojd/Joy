#ifndef JOY_ASSETS_H
#define JOY_ASSETS_H

#include "joy_types.h"
#include "joy_defines.h"
#include "joy_asset_types.h"
#include "joy_asset_util.h"

// NOTE(dima): Bitmaps are stored in gamma-corrected premultiplied alpha format
struct Assets{
    Bmp_Info sunset;
    Bmp_Info sunsetOrange;
    Bmp_Info sunsetField;
    Bmp_Info sunsetMountains;
    Bmp_Info sunsetPurple;
    Bmp_Info sunrise;
    Bmp_Info mountainsFuji;
    Bmp_Info roadClouds;
    
    Bmp_Info checkboxMark;
    
    Font_Info liberationMono;
    Font_Info lilitaOne;
    Font_Info inconsolataBold;
    Font_Info pfdin;
};

void InitAssets(Assets* assets);

#endif