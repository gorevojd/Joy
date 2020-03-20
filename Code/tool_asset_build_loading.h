#ifndef TOOL_ASSET_BUILD_LOADING
#define TOOL_ASSET_BUILD_LOADING

#include "tool_asset_build_types.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <intrin.h>

#define MM(mm, i) (mm).m128_f32[i]
#define MMI(mm, i) (mm).m128i_u32[i]

#define MM_UNPACK_COLOR_CHANNEL(texel, shift) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(texel, shift), mmFF)), mmOneOver255)

#define MM_UNPACK_COLOR_CHANNEL0(texel) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(texel, mmFF)), mmOneOver255)

#define MM_LERP(a, b, t) _mm_add_ps(a, _mm_mul_ps(_mm_sub_ps(b, a), t))

enum load_font_flags{
    LoadFont_BakeShadow = 1,
    LoadFont_BakeBlur = 2,
};

// NOTE(Dima): Data buffers utility functions
data_buffer ReadFileToDataBuffer(char* fileName);
void FreeDataBuffer(data_buffer* dataBuffer);

//NOTE(Dima): Bitmap utility functions
tool_bmp_info AllocateBitmapInternal(u32 width, u32 height, void* pixelsData);
void AllocateBitmapInternal(tool_bmp_info* Bmp, u32 Width, u32 Height, void* PixelsData);
tool_bmp_info AllocateBitmap(u32 width, u32 height);
void DeallocateBitmap(tool_bmp_info* buffer);
void CopyBitmapData(tool_bmp_info* dst, tool_bmp_info* src);

// NOTE(Dima): Functions for loading stuff
tool_sound_info LoadSound(char* FilePath);
tool_bmp_info LoadBMP(char* filePath);
tool_font_info LoadFont(char* FilePath, float height, u32 Flags);
Loaded_Strings LoadStringListFromFile(char* filePath);
void FreeStringList(Loaded_Strings* list);

tool_mesh_info MakeMesh(
std::vector<v3>& Positions,
std::vector<v2>& TexCoords,
std::vector<v3>& Normals,
std::vector<v3>& Tangents,
std::vector<v3>& Colors,
std::vector<u32> Indices,
b32 CalculateNormals,
b32 CalculateTangents);
tool_mesh_info MakePlane();
tool_mesh_info MakeCube();
tool_mesh_info MakeSphere(int Segments, int Rings);
tool_mesh_info MakeCylynder(float Height, float Radius, int SidesCount) ;

tool_sound_info MakeSound(const std::vector<i16>& Samples,
                          int SamplesPerSec);
tool_sound_info MakeSineSound(const std::vector<int>& Frequencies, 
                              int SampleCount,
                              int SamplesPerSec);
tool_sound_info MakeSineSound256(int SampleCount, int SamplesPerSec);

#endif