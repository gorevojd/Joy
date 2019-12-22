#ifndef JOY_ASSET_UTIL_H
#define JOY_ASSET_UTIL_H

#include "joy_math.h"
#include "joy_types.h"
#include "joy_asset_types.h"

struct Data_Buffer {
	u8* data;
	u64 size;
};

enum load_font_flags{
    LoadFont_BakeShadow = 1,
    LoadFont_BakeBlur = 2,
};

// NOTE(Dima): Data buffers utility functions
Data_Buffer ReadFileToDataBuffer(char* fileName);
void FreeDataBuffer(Data_Buffer* dataBuffer);

//NOTE(Dima): Bitmap utility functions
Bmp_Info AssetAllocateBitmapInternal(u32 width, u32 height, void* pixelsData);
Bmp_Info AssetAllocateBitmap(u32 width, u32 height);
void AssetDeallocateBitmap(Bmp_Info* buffer);
void AssetCopyBitmapData(Bmp_Info* dst, Bmp_Info* src);

// NOTE(Dima): Functions for loading stuff
Bmp_Info LoadBMP(char* filePath);
Font_Info LoadFont(char* filePath, float height, u32 flags = 0);


#endif