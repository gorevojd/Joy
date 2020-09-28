#ifndef JOY_ASSET_UTIL_H
#define JOY_ASSET_UTIL_H

#include "joy_math.h"
#include "joy_types.h"
#include "joy_asset_types.h"

inline bmp_info AllocateBitmapInternal(u32 Width, u32 Height, void* pixelsData) {
	bmp_info res = {};
    
    res.Prim.LayoutType = BmpDataLayout_RGBA;
	res.Prim.Data = pixelsData;
	res.Prim.Width = Width;
	res.Prim.Height = Height;
    
	res.Prim.Pitch = 4 * Width;
	res.Prim.WidthOverHeight = (float)Width / (float)Height;
    
	return(res);
}

inline void AllocateBitmapInternal(bmp_info* Bmp, u32 Width, u32 Height, void* PixelsData){
    if(Bmp){
        *Bmp = AllocateBitmapInternal(Width, Height, PixelsData);
    }
}

#endif