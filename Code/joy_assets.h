#ifndef JOY_ASSETS_H
#define JOY_ASSETS_H

#include "joy_types.h"
#include "joy_defines.h"
#include "joy_asset_types.h"
#include "joy_asset_util.h"

// NOTE(dima): Bitmaps are stored in gamma-corrected premultiplied alpha format
struct assets{
    bmp_info Sunset;
    bmp_info SunsetOrange;
    bmp_info SunsetField;
    bmp_info SunsetMountains;
    bmp_info SunsetPurple;
    bmp_info Sunrise;
    bmp_info MountainsFuji;
    bmp_info RoadClouds;
    
    bmp_info CheckboxMark;
    
    font_info LiberationMono;
};

void InitAssets(assets* Assets);

#endif