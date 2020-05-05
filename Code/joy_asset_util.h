#ifndef JOY_ASSET_UTIL_H
#define JOY_ASSET_UTIL_H

#include "joy_math.h"
#include "joy_types.h"
#include "joy_asset_types.h"

inline bmp_info AllocateBitmapInternal(u32 Width, u32 Height, void* pixelsData) {
	bmp_info res = {};
    
	res.Width = Width;
	res.Height = Height;
	res.Pitch = 4 * Width;
    
	res.WidthOverHeight = (float)Width / (float)Height;
    
	res.Pixels = pixelsData;
    
	return(res);
}

inline void AllocateBitmapInternal(bmp_info* Bmp, u32 Width, u32 Height, void* PixelsData){
    if(Bmp){
        *Bmp = AllocateBitmapInternal(Width, Height, PixelsData);
    }
}

#endif