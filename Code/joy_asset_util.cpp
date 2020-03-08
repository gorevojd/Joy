#include "joy_asset_util.h"
#include "joy_software_renderer.h"
#include "joy_defines.h"
#include "joy_render_blur.h"
#include "joy_simd.h"

#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

#include <intrin.h>

data_buffer ReadFileToDataBuffer(char* FileName) {
	data_buffer Result = {};
    
	FILE* fp = fopen(FileName, "rb");
	if (fp) {
		fseek(fp, 0, 2);
		u64 FileSize = ftell(fp);
		fseek(fp, 0, 0);
        
		Result.Size = FileSize;
		Result.Data = (u8*)calloc(FileSize, 1);
        
		fread(Result.Data, 1, FileSize, fp);
        
		fclose(fp);
	}
    
	return(Result);
}

void FreeDataBuffer(data_buffer* DataBuffer) {
	if (DataBuffer->Data) {
		free(DataBuffer->Data);
	}
}


inline b32 IsWhitespaceNewlineOrEndline(char c){
    b32 res = (c == '\n' || 
               c == '\r' ||
               c == ' ' ||
               c == 0);
    
    return(res);
}

Loaded_Strings LoadStringListFromFile(char* filePath){
    Loaded_Strings result = {};
    
    data_buffer buf = ReadFileToDataBuffer(filePath);
    
    std::vector<std::string> strings;
    
    char* at = (char*)buf.Data;
    
    char bufStr[256];
    int inLinePos = 0;
    
    if(buf.Size){
        
        while(at && *at != 0){
            if(IsWhitespaceNewlineOrEndline(*at))
            {
                bufStr[inLinePos] = 0;
                strings.push_back(std::string(bufStr));
                
                inLinePos = 0;
                
                // NOTE(Dima): Skip to next valid symbol
                while(at && IsWhitespaceNewlineOrEndline(*at))
                {
                    if(*at == 0){
                        break;
                    }
                    
                    at++;
                }
            }
            else{
                bufStr[inLinePos++] = *at;
                at++;
            }
        }
        
        result.Count = strings.size();
        
        size_t totalDataNeeded = 0;
        for(int i = 0; i < strings.size(); i++){
            totalDataNeeded += strings[i].length() + 1 + sizeof(char*);
        }
        
        result.Strings = (char**)malloc(totalDataNeeded);
        
        int curSumOfSizes = 0;
        for(int i = 0; i < result.Count; i++){
            result.Strings[i] = (char*)result.Strings + result.Count * sizeof(char*) + curSumOfSizes;
            curSumOfSizes += strings[i].length() + 1;
            strcpy(result.Strings[i], strings[i].c_str());
        }
        
        FreeDataBuffer(&buf);
    }
    
    return(result);
}

void FreeStringList(Loaded_Strings* list){
    if(list->Strings){
        free(list->Strings);
        list->Strings = 0;
    }
}

bmp_info AllocateBitmapInternal(u32 Width, u32 Height, void* pixelsData) {
	bmp_info res = {};
    
	res.Width = Width;
	res.Height = Height;
	res.Pitch = 4 * Width;
    
	res.WidthOverHeight = (float)Width / (float)Height;
    
	res.Pixels = pixelsData;
    
	return(res);
}

void AllocateBitmapInternal(bmp_info* Bmp, u32 Width, u32 Height, void* PixelsData){
    
    if(Bmp){
        *Bmp = AllocateBitmapInternal(Width, Height, PixelsData);
    }
    
}

bmp_info AllocateBitmap(u32 Width, u32 Height) {
	u32 BitmapDataSize = Width * Height * 4;
	void* PixelsData = calloc(BitmapDataSize, 1);
    
	memset(PixelsData, 0, BitmapDataSize);
    
	bmp_info res = AllocateBitmapInternal(Width, Height, PixelsData);
    
	return(res);
}

void CopyBitmapData(bmp_info* Dst, bmp_info* Src) {
	Assert(Dst->Width == Src->Width);
	Assert(Dst->Height == Src->Height);
    
	u32* DestOut = (u32*)Dst->Pixels;
	u32* ScrPix = (u32*)Src->Pixels;
	for (int j = 0; j < Src->Height; j++) {
		for (int i = 0; i < Src->Width; i++) {
			*DestOut++ = *ScrPix++;
		}
	}
}

void DeallocateBitmap(bmp_info* Buffer) {
	if (Buffer->Pixels) {
		free(Buffer->Pixels);
	}
}

sound_info LoadSound(char* FilePath){
    sound_info Result = {};
    
    
    
    return(Result);
}

bmp_info LoadBMP(char* FilePath){
    bmp_info res = {};
    
    int width;
    int height;
    int Channels;
    
    unsigned char* Image = stbi_load(
        FilePath,
        &width,
        &height,
        &Channels,
        STBI_rgb_alpha);
    
    int pixelsCount = width * height;
    int ImageSize = pixelsCount * 4;
    
    void* OurImageMem = malloc(ImageSize);
    res = AllocateBitmapInternal(width, height, OurImageMem);
    
    int pixelsCountForSIMD = pixelsCount & (~3);
    
    __m128 mm255 = _mm_set1_ps(255.0f);
    __m128 mmOneOver255 = _mm_set1_ps(1.0f / 255.0f);
    __m128i mmFF = _mm_set1_epi32(0xFF);
    
#if 1
    int PixelIndex = 0;
    for(PixelIndex;
        PixelIndex < pixelsCountForSIMD;
        PixelIndex += 4)
    {
        u32* Pix = ((u32*)Image + PixelIndex);
        
        __m128i mmTexel = _mm_setr_epi32(*Pix, *(Pix + 1), *(Pix + 2), *(Pix + 3));
        __m128 mmTexel_r = MM_UNPACK_COLOR_CHANNEL0(mmTexel);
        __m128 mmTexel_g = MM_UNPACK_COLOR_CHANNEL(mmTexel, 8);
        __m128 mmTexel_b = MM_UNPACK_COLOR_CHANNEL(mmTexel, 16);
        __m128 mmTexel_a = MM_UNPACK_COLOR_CHANNEL(mmTexel, 24);
        
        mmTexel_r = _mm_mul_ps(mmTexel_r, mmTexel_a);
        mmTexel_g = _mm_mul_ps(mmTexel_g, mmTexel_a);
        mmTexel_b = _mm_mul_ps(mmTexel_b, mmTexel_a);
        
        __m128i mmColorShifted_a = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmTexel_a, mm255)), 24);
        __m128i mmColorShifted_b = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmTexel_b, mm255)), 16);
        __m128i mmColorShifted_g = _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(mmTexel_g, mm255)), 8);
        __m128i mmColorShifted_r = _mm_cvtps_epi32(_mm_mul_ps(mmTexel_r, mm255));
        
        __m128i mmResult = _mm_or_si128(
            _mm_or_si128(mmColorShifted_r, mmColorShifted_g),
            _mm_or_si128(mmColorShifted_b, mmColorShifted_a));
        
        _mm_storeu_si128((__m128i*)((u32*)res.Pixels + PixelIndex), mmResult);
    }
    
    for(PixelIndex;
        PixelIndex < pixelsCount;
        PixelIndex++)
    {
        u32 Pix = *((u32*)Image + PixelIndex);
        
        v4 Color = UnpackRGBA(Pix);
        Color.r *= Color.a;
        Color.g *= Color.a;
        Color.b *= Color.a;
        
        u32 PackedColor = PackRGBA(Color);
        
        *((u32*)res.Pixels + PixelIndex) = PackedColor;
    }
#else
    
    for(int PixelIndex = 0;
        PixelIndex < pixelsCount;
        PixelIndex++)
    {
        u32 Pix = *((u32*)Image + PixelIndex);
        
        v4 Color = UnpackRGBA(Pix);
        Color.r *= Color.a;
        Color.g *= Color.a;
        Color.b *= Color.a;
        
        u32 PackedColor = PackRGBA(Color);
        
        *((u32*)res.Pixels + PixelIndex) = PackedColor;
    }
#endif
    
    
    stbi_image_free(Image);
    
    return(res);
}


mesh_info MakeMesh(
std::vector<v3>& Positions,
std::vector<v2>& TexCoords,
std::vector<v3>& Normals,
std::vector<v3>& Tangents,
std::vector<v3>& Colors,
std::vector<u32> Indices,
b32 CalculateNormals,
b32 CalculateTangents)
{
	mesh_info Result = {};
    
    u32 VerticesCount = Positions.size();
    
	Result.IndicesCount = Indices.size();
	Result.Indices = (u32*)malloc(Indices.size() * sizeof(u32));
	for (int IndexIndex = 0;
         IndexIndex < Indices.size();
         IndexIndex++)
	{
		Result.Indices[IndexIndex] = Indices[IndexIndex];
	}
    
    Result.MeshType = Mesh_Simple;
	Result.VerticesCount = VerticesCount;
    size_t VertsSize = sizeof(vertex_info) * VerticesCount;
	Result.Vertices = malloc(VertsSize);
    memset(Result.Vertices, 0, VertsSize);
    
    ASSERT(Positions.size());
    ASSERT(TexCoords.size());
    
    vertex_info* DstVerts = (vertex_info*)Result.Vertices;
    
    for (int Index = 0;
         Index < Result.IndicesCount;
         Index += 3)
    {
        int Index0 = Result.Indices[Index];
        int Index1 = Result.Indices[Index + 1];
        int Index2 = Result.Indices[Index + 2];
        
        v3 P0 = Positions[Index0];
        v3 P1 = Positions[Index1];
        v3 P2 = Positions[Index2];
        
        v2 Tex0 = TexCoords[Index0];
        v2 Tex1 = TexCoords[Index1];
        v2 Tex2 = TexCoords[Index2];
        
        DstVerts[Index0].P = P0;
        DstVerts[Index1].P = P1;
        DstVerts[Index2].P = P2;
        
        DstVerts[Index0].UV = Tex0;
        DstVerts[Index1].UV = Tex1;
        DstVerts[Index2].UV = Tex2;
        
        v3 Edge1 = P1 - P0;
        v3 Edge2 = P2 - P0;
        
        if (CalculateTangents || (Tangents.size() != VerticesCount)) {
            v2 DeltaTex1 = Tex1 - Tex0;
            v2 DeltaTex2 = Tex2 - Tex0;
            
            float InvDet = 1.0f / (DeltaTex1.x * DeltaTex2.y - DeltaTex2.x * DeltaTex1.y);
            
            v3 T = InvDet * (DeltaTex2.y * Edge1 - DeltaTex1.y * Edge2);
            v3 B = InvDet * (DeltaTex1.x * Edge2 - DeltaTex2.x * Edge1);
            
            T = Normalize(T);
            /*
            NOTE(dima): bitangent calculation is implemented
            but not used...
            */
            B = Normalize(B);
            
            //NOTE(dima): Setting the calculating tangent to the vertex;
            DstVerts[Index0].T = T;
            DstVerts[Index1].T = T;
            DstVerts[Index2].T = T;
        }
        else{
            //NOTE(dima): Just copy tangents if they exist
            DstVerts[Index0].T = Tangents[Index0];
            DstVerts[Index1].T = Tangents[Index1];
            DstVerts[Index2].T = Tangents[Index2];
        }
        
        //NOTE(dima): Normals calculation and setting
        if (CalculateNormals || (Normals.size() != VerticesCount)) {
            v3 TriNormal = Normalize(Cross(Edge2, Edge1));
            
            DstVerts[Index0].N = TriNormal;
            DstVerts[Index1].N = TriNormal;
            DstVerts[Index2].N = TriNormal;
        }
        else{
            DstVerts[Index0].N = Normals[Index0];
            DstVerts[Index1].N = Normals[Index1];
            DstVerts[Index2].N = Normals[Index2];
        }
        
        // NOTE(Dima): Colors copying
        if(Colors.size() != VerticesCount){
            DstVerts[Index0].C = V3(0.0f, 0.0f, 0.0f);
            DstVerts[Index1].C = V3(0.0f, 0.0f, 0.0f);
            DstVerts[Index2].C = V3(0.0f, 0.0f, 0.0f);
        }
        else{
            DstVerts[Index0].C = Colors[Index0];
            DstVerts[Index1].C = Colors[Index1];
            DstVerts[Index2].C = Colors[Index2];
        }
    }
    
    return(Result);
}

mesh_info MakePlane(){
    mesh_info Result = {};
    
    std::vector<v3> Positions;
    std::vector<v2> TexCoords;
    std::vector<v3> Normals;
    std::vector<v3> Colors;
    std::vector<u32> Indices;
    
    int VertAt = 0;
    
    // NOTE(Dima): Pushing plane
    v3 Ps[] = {
        V3(-0.5f, 0.0f, -0.5f),
        V3(0.5f, 0.0f, -0.5f),
        V3(0.5f, 0.0f, 0.5f),
        V3(-0.5f, 0.0f, 0.5f),
    };
    
    v3 Cs[] = {
        V3(1.0f, 0.0f, 0.0f),
        V3(0.0f, 1.0f, 0.0f),
        V3(0.0f, 0.0f, 1.0f),
        V3(1.0f, 1.0f, 0.0f),
        V3(1.0f, 0.0f, 1.0f),
        V3(0.0f, 1.0f, 1.0f),
        V3(1.0f, 0.5f, 0.0f),
        V3(0.5f, 1.0f, 0.2f),
    };
    
    v2 T0 = V2(0.0f, 1.0f);
    v2 T1 = V2(1.0f, 1.0f);
    v2 T2 = V2(1.0f, 0.0f);
    v2 T3 = V2(0.0f, 0.0f);
    
    
    Positions.push_back(Ps[0]);
    Positions.push_back(Ps[1]);
    Positions.push_back(Ps[2]);
    Positions.push_back(Ps[3]);
    
    Colors.push_back(Cs[0]);
    Colors.push_back(Cs[1]);
    Colors.push_back(Cs[2]);
    Colors.push_back(Cs[3]);
    
    v3 n = V3(0.0f, 1.0f, 0.0f);
    Normals.push_back(n);
    Normals.push_back(n);
    Normals.push_back(n);
    Normals.push_back(n);
    
    TexCoords.push_back(T0);
    TexCoords.push_back(T1);
    TexCoords.push_back(T2);
    TexCoords.push_back(T3);
    
    Indices.push_back(0);
    Indices.push_back(1);
    Indices.push_back(2);
    
    Indices.push_back(0);
    Indices.push_back(2);
    Indices.push_back(3);
    
    Result = MakeMesh(Positions, 
                      TexCoords, 
                      Normals, 
                      std::vector<v3>(), 
                      Colors,
                      Indices,
                      JOY_FALSE, 
                      JOY_TRUE);
    
    return(Result);
}

INTERNAL_FUNCTION inline 
void PushSide(std::vector<v3>& Positions,
              std::vector<v2>& TexCoords,
              std::vector<v3>& Normals,
              std::vector<v3>& Colors,
              std::vector<u32>& Indices,
              int i1, int i2, int i3, int i4,
              v3 n, int* Increment)
{
    v3 Ps[] = {
        V3(-0.5f, 0.5f, 0.5f),
        V3(0.5f, 0.5f, 0.5f),
        V3(0.5f, -0.5f, 0.5f),
        V3(-0.5f, -0.5f, 0.5f),
        V3(-0.5f, 0.5f, -0.5f),
        V3(0.5f, 0.5f, -0.5f),
        V3(0.5f, -0.5f, -0.5f),
        V3(-0.5f, -0.5f, -0.5f),
    };
    
    v3 Cs[] = {
        V3(1.0f, 0.0f, 0.0f),
        V3(0.0f, 1.0f, 0.0f),
        V3(0.0f, 0.0f, 1.0f),
        V3(1.0f, 1.0f, 0.0f),
        V3(1.0f, 0.0f, 1.0f),
        V3(0.0f, 1.0f, 1.0f),
        V3(1.0f, 0.5f, 0.0f),
        V3(0.5f, 1.0f, 0.2f),
    };
    
    v2 T0 = V2(0.0f, 1.0f);
    v2 T1 = V2(1.0f, 1.0f);
    v2 T2 = V2(1.0f, 0.0f);
    v2 T3 = V2(0.0f, 0.0f);
    
    Positions.push_back(Ps[i1]);
    Positions.push_back(Ps[i2]);
    Positions.push_back(Ps[i3]);
    Positions.push_back(Ps[i4]);
    
    Colors.push_back(Cs[i1]);
    Colors.push_back(Cs[i2]);
    Colors.push_back(Cs[i3]);
    Colors.push_back(Cs[i4]);
    
    Normals.push_back(n);
    Normals.push_back(n);
    Normals.push_back(n);
    Normals.push_back(n);
    
    TexCoords.push_back(T0);
    TexCoords.push_back(T1);
    TexCoords.push_back(T2);
    TexCoords.push_back(T3);
    
    Indices.push_back(*Increment);
    Indices.push_back(*Increment + 1);
    Indices.push_back(*Increment + 2);
    
    Indices.push_back(*Increment);
    Indices.push_back(*Increment + 2);
    Indices.push_back(*Increment + 3);
    *Increment += 4;
    
}

mesh_info MakeCube(){
    mesh_info Result = {};
    
    std::vector<v3> Positions;
    std::vector<v2> TexCoords;
    std::vector<v3> Normals;
    std::vector<v3> Colors;
    std::vector<u32> Indices;
    
    v3 Front = V3(0.0f, 0.0f, 1.0f);
    v3 Back = V3(0.0f, 0.0f, -1.0f);
    v3 Up = V3(0.0f, 1.0f, 0.0f);
    v3 Down = V3(0.0f, -1.0f, 0.0f);
    v3 Left = V3(1.0f, 0.0f, 0.0f);
    v3 Right = V3(-1.0f, 0.0f, 0.0f);
    
    int VertAt = 0;
    
    // NOTE(Dima): Pushing top
    PushSide(Positions, TexCoords, 
             Normals, Colors, Indices,
             4, 5, 1, 0,
             Up, &VertAt);
    
    // NOTE(Dima): Pushing bottom
    PushSide(Positions, TexCoords,
             Normals, Colors, Indices,
             3, 2, 6, 7,
             Down, &VertAt);
    
    // NOTE(Dima): Pushing front
    PushSide(Positions, TexCoords,
             Normals, Colors, Indices,
             0, 1, 2, 3,
             Front, &VertAt);
    
    // NOTE(Dima): Pushing back
    PushSide(Positions, TexCoords,
             Normals, Colors, Indices,
             5, 4, 7, 6,
             Back, &VertAt);
    
    // NOTE(Dima): Pushing left
    PushSide(Positions, TexCoords,
             Normals, Colors, Indices,
             1, 5, 6, 2,
             Left, &VertAt);
    
    // NOTE(Dima): Pushing right
    PushSide(Positions, TexCoords,
             Normals, Colors, Indices,
             4, 0, 3, 7,
             Right, &VertAt);
    
    Result = MakeMesh(Positions, 
                      TexCoords, 
                      Normals, 
                      std::vector<v3>(), 
                      Colors,
                      Indices,
                      JOY_FALSE, 
                      JOY_TRUE);
    
    return(Result);
}

mesh_info MakeSphere(int Segments, int Rings) {
    mesh_info Result = {};
    
    float Radius = 0.5f;
    
    Segments = Max(Segments, 3);
    Rings = Max(Rings, 2);
    
    //NOTE(dima): 2 top and bottom triangle fans + 
    int VerticesCount = (Segments * 3) * 2 + (Segments * (Rings - 2)) * 4;
    int IndicesCount = (Segments * 3) * 2 + (Segments * (Rings - 2)) * 6;
    
    v3 Color = V3(1.0f);
    
    std::vector<v3> Positions;
    std::vector<v2> TexCoords;
    std::vector<v3> Normals;
    std::vector<v3> Colors;
    std::vector<u32> Indices;
    
    float AngleVert = JOY_PI / (float)Rings;
    float AngleHorz = JOY_TWO_PI / (float)Segments;
    
    int VertexAt = 0;
    int IndexAt = 0;
    
    for (int VertAt = 1; VertAt <= Rings; VertAt++) {
        float CurrAngleVert = (float)VertAt * AngleVert;
        float PrevAngleVert = (float)(VertAt - 1) * AngleVert;
        
        float PrevY = Cos(PrevAngleVert) * Radius;
        float CurrY = Cos(CurrAngleVert) * Radius;
        
        float SinVertPrev = Sin(PrevAngleVert);
        float SinVertCurr = Sin(CurrAngleVert);
        
        for (int HorzAt = 1; HorzAt <= Segments; HorzAt++) {
            float CurrAngleHorz = (float)HorzAt * AngleHorz;
            float PrevAngleHorz = (float)(HorzAt - 1) * AngleHorz;
            
            v3 P0, P1, C0, C1;
            v2 P0uv, P1uv, C0uv, C1uv;
            
            P0.y = PrevY;
            P1.y = PrevY;
            
            C0.y = CurrY;
            C1.y = CurrY;
            
            //TODO(dima): handle triangle fan case
            P0.x = Cos(PrevAngleHorz) * SinVertPrev * Radius;
            P1.x = Cos(CurrAngleHorz) * SinVertPrev * Radius;
            
            P0.z = Sin(PrevAngleHorz) * SinVertPrev * Radius;
            P1.z = Sin(CurrAngleHorz) * SinVertPrev * Radius;
            
            C0.x = Cos(PrevAngleHorz) * SinVertCurr * Radius;
            C1.x = Cos(CurrAngleHorz) * SinVertCurr * Radius;
            
            C0.z = Sin(PrevAngleHorz) * SinVertCurr * Radius;
            C1.z = Sin(CurrAngleHorz) * SinVertCurr * Radius;
            
            v3 NP0 = Normalize(P0);
            v3 NP1 = Normalize(P1);
            v3 NC0 = Normalize(C0);
            v3 NC1 = Normalize(C1);
            
            P0uv = V2(0.0f, 0.0f);
            P1uv = V2(0.0f, 0.0f);
            C0uv = V2(0.0f, 0.0f);
            C1uv = V2(0.0f, 0.0f);
            
            if (VertAt == 1) {
                // NOTE(Dima): Top fan
                Positions.push_back(P0);
                Positions.push_back(C0);
                Positions.push_back(C1);
                
                TexCoords.push_back(P0uv);
                TexCoords.push_back(C0uv);
                TexCoords.push_back(C1uv);
                
                Normals.push_back(NP0);
                Normals.push_back(NC0);
                Normals.push_back(NC1);
                
                Colors.push_back(Color);
                Colors.push_back(Color);
                Colors.push_back(Color);
                
                Indices.push_back(VertexAt);
                Indices.push_back(VertexAt + 1);
                Indices.push_back(VertexAt + 2);
                
                IndexAt += 3;
                VertexAt += 3;
            }
            else if (VertAt == Rings) {
                // NOTE(Dima): Bottom fan
                Positions.push_back(P1);
                Positions.push_back(P0);
                Positions.push_back(C1);
                
                TexCoords.push_back(P1uv);
                TexCoords.push_back(P0uv);
                TexCoords.push_back(C1uv);
                
                Normals.push_back(NP1);
                Normals.push_back(NP0);
                Normals.push_back(NC1);
                
                Colors.push_back(Color);
                Colors.push_back(Color);
                Colors.push_back(Color);
                
                Indices.push_back(VertexAt);
                Indices.push_back(VertexAt + 1);
                Indices.push_back(VertexAt + 2);
                
                IndexAt += 3;
                VertexAt += 3;
            }
            else {
                Positions.push_back(P1);
                Positions.push_back(P0);
                Positions.push_back(C0);
                Positions.push_back(C1);
                
                TexCoords.push_back(P1uv);
                TexCoords.push_back(P0uv);
                TexCoords.push_back(C0uv);
                TexCoords.push_back(C1uv);
                
                Normals.push_back(NP1);
                Normals.push_back(NP0);
                Normals.push_back(NC0);
                Normals.push_back(NC1);
                
                Colors.push_back(Color);
                Colors.push_back(Color);
                Colors.push_back(Color);
                Colors.push_back(Color);
                
                Indices.push_back(VertexAt);
                Indices.push_back(VertexAt + 1);
                Indices.push_back(VertexAt + 2);
                Indices.push_back(VertexAt);
                Indices.push_back(VertexAt + 2);
                Indices.push_back(VertexAt + 3);
                
                IndexAt += 6;
                VertexAt += 4;
            }
        }
    }
    
    ASSERT(Positions.size() == VerticesCount);
    ASSERT(TexCoords.size() == VerticesCount);
    ASSERT(Normals.size() == VerticesCount);
    ASSERT(Indices.size() == IndicesCount);
    
    Result = MakeMesh(Positions, 
                      TexCoords, 
                      Normals, 
                      std::vector<v3>(), 
                      Colors,
                      Indices,
                      JOY_FALSE, 
                      JOY_TRUE);
    
    return(Result);
}

mesh_info MakeCylynder(float Height, float Radius, int SidesCount) {
    mesh_info Result = {};
    
    SidesCount = Max(3, SidesCount);
    
    int VerticesCount = SidesCount * 4 + SidesCount * 2 * 3;
    int IndicesCount = SidesCount * 6 + SidesCount * 2 * 3;
    
    float Angle = JOY_TWO_PI / (float)SidesCount;
    
    int IndexAt = 0;
    int VertexAt = 0;
    
    std::vector<v3> Positions;
    std::vector<v2> TexCoords;
    std::vector<v3> Normals;
    std::vector<u32> Indices;
    std::vector<v3> Colors;
    
    v3 Color = V3(1.0f);
    
    //NOTE(dima): Building top triangle fans
    float TopY = Height * 0.5f;
    for (int Index = 1;
         Index <= SidesCount;
         Index++)
    {
        float CurrAngle = (float)Index * Angle;
        float PrevAngle = (float)(Index - 1) * Angle;
        
        float TopY = Height * 0.5f;
        
        v3 CurrP;
        v3 PrevP;
        v3 Center = V3(0.0f, 0.0f, 0.0f);
        
        CurrP.x = Cos(CurrAngle) * Radius;
        CurrP.y = TopY;
        CurrP.z = Sin(CurrAngle) * Radius;
        
        PrevP.x = Cos(PrevAngle) * Radius;
        PrevP.y = TopY;
        PrevP.z = Sin(PrevAngle) * Radius;
        
        v2 CurrUV = V2(0.0f, 0.0f);
        v2 PrevUV = V2(0.0f, 0.0f);
        v2 CentUV = V2(0.0f, 0.0f);
        
        Center.y = TopY;
        
        // NOTE(Dima): Pushing vertex data
        Positions.push_back(PrevP);
        Positions.push_back(CurrP);
        Positions.push_back(Center);
        
        TexCoords.push_back(PrevUV);
        TexCoords.push_back(CurrUV);
        TexCoords.push_back(CentUV);
        
        v3 Normal = V3(0.0f, 1.0f, 0.0f);
        Normals.push_back(Normal);
        Normals.push_back(Normal);
        Normals.push_back(Normal);
        
        Colors.push_back(Color);
        Colors.push_back(Color);
        Colors.push_back(Color);
        
        // NOTE(Dima): Pushing indices
        Indices.push_back(VertexAt);
        Indices.push_back(VertexAt + 1);
        Indices.push_back(VertexAt + 2);
        
        VertexAt += 3;
        IndexAt += 3;
    }
    
    //NOTE(dima): Building bottom triangle fans
    for (int Index = 1;
         Index <= SidesCount;
         Index++)
    {
        float CurrAngle = (float)Index * Angle;
        float PrevAngle = (float)(Index - 1) * Angle;
        
        float BotY = -Height * 0.5f;
        
        v3 CurrP;
        v3 PrevP;
        v3 Center = V3(0.0f, 0.0f, 0.0f);
        
        CurrP.x = Cos(CurrAngle) * Radius;
        CurrP.y = BotY;
        CurrP.z = Sin(CurrAngle) * Radius;
        
        PrevP.x = Cos(PrevAngle) * Radius;
        PrevP.y = BotY;
        PrevP.z = Sin(PrevAngle) * Radius;
        
        v2 CurrUV = V2(0.0f, 0.0f);
        v2 PrevUV = V2(0.0f, 0.0f);
        v2 CentUV = V2(0.0f, 0.0f);
        
        Center.y = BotY;
        
        // NOTE(Dima): Pushing vertex data
        Positions.push_back(CurrP);
        Positions.push_back(PrevP);
        Positions.push_back(Center);
        
        TexCoords.push_back(CurrUV);
        TexCoords.push_back(PrevUV);
        TexCoords.push_back(CentUV);
        
        v3 Normal = V3(0.0f, -1.0f, 0.0f);
        Normals.push_back(Normal);
        Normals.push_back(Normal);
        Normals.push_back(Normal);
        
        Colors.push_back(Color);
        Colors.push_back(Color);
        Colors.push_back(Color);
        
        
        // NOTE(Dima): Pushing indices
        Indices.push_back(VertexAt);
        Indices.push_back(VertexAt + 1);
        Indices.push_back(VertexAt + 2);
        
        VertexAt += 3;
        IndexAt += 3;
    }
    
    //NOTE(dima): Building sides
    for (int Index = 1;
         Index <= SidesCount;
         Index++)
    {
        float CurrAngle = (float)Index * Angle;
        float PrevAngle = (float)(Index - 1) * Angle;
        
        v3 CurrP;
        v3 PrevP;
        
        CurrP.x = Cos(CurrAngle) * Radius;
        CurrP.y = 0.0f;
        CurrP.z = Sin(CurrAngle) * Radius;
        
        PrevP.x = Cos(PrevAngle) * Radius;
        PrevP.y = 0.0f;
        PrevP.z = Sin(PrevAngle) * Radius;
        
        v3 TopC, TopP;
        v3 BotC, BotP;
        
        v2 TopCuv = V2(0.0f, 0.0f);
        v2 TopPuv = V2(0.0f, 0.0f);
        v2 BotCuv = V2(0.0f, 0.0f);
        v2 BotPuv = V2(0.0f, 0.0f);
        
        TopC = CurrP;
        BotC = CurrP;
        TopP = PrevP;
        BotP = PrevP;
        
        v3 Normal = Normalize(TopC);
        
        TopC.y = Height * 0.5f;
        TopP.y = Height * 0.5f;
        BotC.y = -Height * 0.5f;
        BotP.y = -Height * 0.5f;
        
        // NOTE(Dima): Pushing vertex data
        Positions.push_back(TopC);
        Positions.push_back(TopP);
        Positions.push_back(BotP);
        Positions.push_back(BotC);
        
        TexCoords.push_back(TopCuv);
        TexCoords.push_back(TopPuv);
        TexCoords.push_back(BotPuv);
        TexCoords.push_back(BotCuv);
        
        Normals.push_back(Normal);
        Normals.push_back(Normal);
        Normals.push_back(Normal);
        Normals.push_back(Normal);
        
        Colors.push_back(Color);
        Colors.push_back(Color);
        Colors.push_back(Color);
        Colors.push_back(Color);
        
        // NOTE(Dima): Pushing indices
        Indices.push_back(VertexAt);
        Indices.push_back(VertexAt + 1);
        Indices.push_back(VertexAt + 2);
        Indices.push_back(VertexAt);
        Indices.push_back(VertexAt + 2);
        Indices.push_back(VertexAt + 3);
        
        VertexAt += 4;
        IndexAt += 6;
    }
    
    Result = MakeMesh(Positions, 
                      TexCoords, 
                      Normals, 
                      std::vector<v3>(), 
                      Colors,
                      Indices,
                      JOY_FALSE, 
                      JOY_TRUE);
    
    return(Result);
}

sound_info MakeSound(const std::vector<i16>& Samples,
                     int SamplesPerSec)
{
    sound_info Result = {};
    
    Result.SamplesPerSec = SamplesPerSec;
    Result.Channels = 2;
    
    if(Samples.size()){
        
        size_t MemNeeded = Samples.size() * sizeof(i16) * Result.Channels;
        i16* ResultSamples = (i16*)malloc(MemNeeded);
        
        Result.Samples[0] = ResultSamples;
        Result.Samples[1] = ResultSamples + Samples.size();
        
        Result.SampleCount = Samples.size();
        
        for(int i = 0; i < Samples.size(); i++){
            *(Result.Samples[0] + i) = Samples[i];
            *(Result.Samples[1] + i) = Samples[i];
        }
    }
    
    return(Result);
}

sound_info MakeSineSound(const std::vector<int>& Frequencies, 
                         int SampleCount, 
                         int SamplesPerSec)
{
    std::vector<i16> Samples;
    Samples.reserve(SampleCount);
    
    if(Frequencies.size()){
        
        float MasterVolume = 0.5f;
        
        float OneOverFreqCount = 1.0f / Frequencies.size();
        
        for(int SampleIndex = 0; SampleIndex < SampleCount; SampleIndex++){
            float ResultSumAmpl = 0.0f;
            
            for(int FreqIndex = 0; FreqIndex < Frequencies.size(); FreqIndex++){
                float SamplesPerPeriod = ((float)SamplesPerSec / (float)Frequencies[FreqIndex]);
                float Phase = (float)SampleIndex * 2.0f * 3.14159265359f / SamplesPerPeriod;
                
                ResultSumAmpl += sinf(Phase);
            }
            
            ResultSumAmpl = ResultSumAmpl * OneOverFreqCount * MasterVolume;
            
#if 1
            i16 ResultSample = (i16)std::roundf(ResultSumAmpl * INT16_MAX);
#else
            if(ResultSumAmpl > 0){
                ResultSample = (i16)(ResultSumAmpl * (float)INT16_MAX + 0.5f);
            }
            else{
                ResultSample = (i16)(ResultSumAmpl * (float)(-INT16_MIN) - 0.5f);
            }
#endif
            
            Samples.push_back(ResultSample);
        }
    }
    
    sound_info Result = MakeSound(Samples, SamplesPerSec);
    
    return(Result);
}

sound_info MakeSineSound256(int SampleCount, int SamplesPerSec){
    std::vector<int> Freq{256};
    
    sound_info Result = MakeSineSound(Freq, SampleCount, SamplesPerSec);
    
    return(Result);
}

