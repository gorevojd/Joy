#ifndef JOY_ASSET_UTIL_H
#define JOY_ASSET_UTIL_H

#include "joy_math.h"
#include "joy_types.h"
#include "joy_asset_types.h"

struct data_buffer {
	u8* Data;
	u64 Size;
};

enum load_font_flags{
    LoadFont_BakeShadow = 1,
    LoadFont_Blur = 2,
};

// NOTE(Dima): Data buffers utility functions
data_buffer ReadFileToDataBuffer(char* FileName);
void FreeDataBuffer(data_buffer* DataBuffer);

//NOTE(Dima): Bitmap utility functions
bmp_info AssetAllocateBitmapInternal(u32 Width, u32 Height, void* PixelsData);
bmp_info AssetAllocateBitmap(u32 Width, u32 Height);
void AssetDeallocateBitmap(bmp_info* Buffer);
void AssetCopyBitmapData(bmp_info* Dst, bmp_info* Src);

// NOTE(Dima): Functions for loading stuff
bmp_info LoadBMP(char* FilePath);
font_info LoadFont(char* FilePath, float Height, u32 Flags = 0);


#endif