#ifndef JOY_ASSET_UTIL_H
#define JOY_ASSET_UTIL_H

#include <vector>

#include "joy_math.h"
#include "joy_types.h"
#include "joy_asset_types.h"

struct data_buffer {
	u8* Data;
	u64 Size;
};

enum load_font_flags{
    LoadFont_BakeShadow = 1,
    LoadFont_BakeBlur = 2,
};

// NOTE(Dima): Data buffers utility functions
data_buffer ReadFileToDataBuffer(char* fileName);
void FreeDataBuffer(data_buffer* dataBuffer);

//NOTE(Dima): Bitmap utility functions
bmp_info AllocateBitmapInternal(u32 width, u32 height, void* pixelsData);
void AllocateBitmapInternal(bmp_info* Bmp, u32 Width, u32 Height, void* PixelsData);
bmp_info AllocateBitmap(u32 width, u32 height);
void DeallocateBitmap(bmp_info* buffer);
void CopyBitmapData(bmp_info* dst, bmp_info* src);

// NOTE(Dima): Functions for loading stuff
sound_info LoadSound(char* FilePath);
bmp_info LoadBMP(char* filePath);
Loaded_Strings LoadStringListFromFile(char* filePath);
void FreeStringList(Loaded_Strings* list);

// NOTE(Dima): Mesh helpers
mesh_info MakeMesh(
std::vector<v3>& Positions,
std::vector<v2>& TexCoords,
std::vector<v3>& Normals,
std::vector<v3>& Tangents,
std::vector<v3>& Colors,
std::vector<u32> Indices,
b32 CalculateNormals,
b32 CalculateTangents);
mesh_info MakePlane();
mesh_info MakeCube();
mesh_info MakeSphere(int Segments, int Rings);
mesh_info MakeCylynder(float Height, float Radius, int SidesCount) ;

sound_info MakeSound(const std::vector<i16>& Samples,
                     int SamplesPerSec);
sound_info MakeSineSound(const std::vector<int>& Frequencies, 
                         int SampleCount,
                         int SamplesPerSec);
sound_info MakeSineSound256(int SampleCount, int SamplesPerSec);

#endif