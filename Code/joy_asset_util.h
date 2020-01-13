#ifndef JOY_ASSET_UTIL_H
#define JOY_ASSET_UTIL_H

#include <vector>

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

struct Loaded_Strings{
    char** strings;
    int count;
};

// NOTE(Dima): Data buffers utility functions
Data_Buffer ReadFileToDataBuffer(char* fileName);
void FreeDataBuffer(Data_Buffer* dataBuffer);

//NOTE(Dima): Bitmap utility functions
Bmp_Info AllocateBitmapInternal(u32 width, u32 height, void* pixelsData);
Bmp_Info AllocateBitmap(u32 width, u32 height);
void DeallocateBitmap(Bmp_Info* buffer);
void CopyBitmapData(Bmp_Info* dst, Bmp_Info* src);

// NOTE(Dima): Functions for loading stuff
Bmp_Info LoadBMP(char* filePath);
Font_Info LoadFont(char* filePath, float height, u32 flags = 0);
Loaded_Strings LoadStringListFromFile(char* filePath);
void FreeStringList(Loaded_Strings* list);

// NOTE(Dima): Mesh helpers
Mesh_Info MakeMesh(
std::vector<v3>& Positions,
std::vector<v2>& TexCoords,
std::vector<v3>& Normals,
std::vector<v3>& Tangents,
std::vector<v3>& Colors,
std::vector<u32> Indices,
b32 CalculateNormals,
b32 CalculateTangents);

Mesh_Info MakePlane();

Mesh_Info MakeCube();

Mesh_Info MakeSphere(int Segments, int Rings);

Mesh_Info MakeCylynder(float Height, float Radius, int SidesCount) ;

#endif