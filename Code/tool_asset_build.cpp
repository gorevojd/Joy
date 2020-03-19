#include "tool_asset_build.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <intrin.h>

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"


#define STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_STATIC
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

#define MM(mm, i) (mm).m128_f32[i]
#define MMI(mm, i) (mm).m128i_u32[i]

#define MM_UNPACK_COLOR_CHANNEL(texel, shift) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(texel, shift), mmFF)), mmOneOver255)

#define MM_UNPACK_COLOR_CHANNEL0(texel) _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(texel, mmFF)), mmOneOver255)

#define MM_LERP(a, b, t) _mm_add_ps(a, _mm_mul_ps(_mm_sub_ps(b, a), t))

//ASSET UTIL
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

bmp_info AllocateBitmapInternal(u32 Width, u32 Height, void* pixelsData) {
	bmp_info res = {};
    
	res.Width = Width;
	res.Height = Height;
	res.Pitch = 4 * Width;
    
	res.WidthOverHeight = (float)Width / (float)Height;
    
	res.Pixels = pixelsData;
    
	return(res);
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


sound_info LoadSound(char* FilePath){
    sound_info Result = {};
    
    
    
    return(Result);
}


//BLUR RENDERING STUFF
u32 Calcualte2DGaussianBoxComponentsCount(int Radius) {
	int Diameter = Radius + Radius + 1;
    
	u32 Result = Diameter * Diameter;
    
	return(Result);
}

void Normalize2DGaussianBox(float* Box, int Radius) {
	//NOTE(dima): Calculate sum of all elements
	float TempSum = 0.0f;
	int Diam = Radius + Radius + 1;
	for (int i = 0; i < Diam * Diam; i++) {
		TempSum += Box[i];
	}
    
	//NOTE(dima): Normalize elements
	float NormValueMul = 1.0f / TempSum;
    
	for (int i = 0; i < Diam * Diam; i++) {
		Box[i] *= NormValueMul;
	}
}

void Calculate2DGaussianBox(float* Box, int Radius) {
	int Diameter = Radius + Radius + 1;
    
	int Center = Radius;
    
	float Sigma = (float)Radius;
    
	float A = 1.0f / (2.0f * JOY_PI * Sigma * Sigma);
    
	float InExpDivisor = 2.0f * Sigma * Sigma;
    
	float TempSum = 0.0f;
    
	//NOTE(dima): Calculate elements
	for (int y = 0; y < Diameter; y++) {
		int PsiY = y - Center;
		for (int x = 0; x < Diameter; x++) {
			int PsiX = x - Center;
            
			float ValueToExp = -((PsiX * PsiX + PsiY * PsiY) / InExpDivisor);
            
			float Expon = Exp(ValueToExp);
            
			float ResultValue = A * Expon;
            
			Box[y * Diameter + x] = ResultValue;
			TempSum += ResultValue;
		}
	}
    
	//NOTE(dima): Normalize elements
	float NormValueMul = 1.0f / TempSum;
    
	for (int i = 0; i < Diameter * Diameter; i++) {
		Box[i] *= NormValueMul;
	}
}

static void BoxBlurApproximate(
bmp_info* To,
bmp_info* From,
int BlurRadius)
{
	int BlurDiam = 1 + BlurRadius + BlurRadius;
    
	for (int Y = 0; Y < From->Height; Y++) {
		for (int X = 0; X < From->Width; X++) {
            
			u32* TargetPixel = (u32*)((u8*)To->Pixels + Y * To->Pitch + X * 4);
            
			v4 VertSum = {};
			int VertSumCount = 0;
			for (int kY = Y - BlurRadius; kY <= Y + BlurRadius; kY++) {
				int targetY = Clamp(kY, 0, From->Height - 1);
                
				u32* ScanPixel = (u32*)((u8*)From->Pixels + targetY * From->Pitch + X * 4);
				v4 UnpackedColor = UnpackRGBA(*ScanPixel);
                
				VertSum += UnpackedColor;
                
				VertSumCount++;
			}
            
            
			v4 HorzSum = {};
			int HorzSumCount = 0;
			for (int kX = X - BlurRadius; kX <= X + BlurRadius; kX++) {
				int targetX = Clamp(kX, 0, From->Width - 1);
                
				u32* ScanPixel = (u32*)((u8*)From->Pixels + Y * From->Pitch + targetX * 4);
				v4 UnpackedColor = UnpackRGBA(*ScanPixel);
                
				HorzSum += UnpackedColor;
                
				HorzSumCount++;
			}
            
            
			VertSum = VertSum / (float)VertSumCount;
			HorzSum = HorzSum / (float)HorzSumCount;
            
			v4 TotalSum = (VertSum + HorzSum) * 0.5f;
            
			*TargetPixel = PackRGBA(TotalSum);
		}
	}
}

bmp_info BlurBitmapApproximateGaussian(
bmp_info* BitmapToBlur,
void* ResultBitmapMem,
void* TempBitmapMem,
int width, int height,
int BlurRadius)
{
	Assert(width == BitmapToBlur->Width);
	Assert(height == BitmapToBlur->Height);
    
	bmp_info Result = AllocateBitmapInternal(
		BitmapToBlur->Width,
		BitmapToBlur->Height,
		ResultBitmapMem);
    
	bmp_info TempBitmap = AllocateBitmapInternal(
		BitmapToBlur->Width,
		BitmapToBlur->Height,
		TempBitmapMem);
    
    
	/*
 var wIdeal = Math.sqrt((12 * sigma*sigma / n) + 1);  // Ideal averaging filter width
 var wl = Math.floor(wIdeal);  if (wl % 2 == 0) wl--;
 var wu = wl + 2;
 var mIdeal = (12 * sigma*sigma - n*wl*wl - 4 * n*wl - 3 * n) / (-4 * wl - 4);
 var m = Math.round(mIdeal);
 // var sigmaActual = Math.sqrt( (m*wl*wl + (n-m)*wu*wu - n)/12 );
 var sizes = [];  for (var i = 0; i<n; i++) sizes.push(i<m ? wl : wu);
 */
    
    
	float Boxes[3];
	int n = 3;
	float nf = 3.0f;
    
	float Sigma = (float)BlurRadius;
	float WIdeal = Sqrt((12.0f * Sigma * Sigma / nf) + 1.0f);
	float wlf = floorf(WIdeal);
	int wl = (float)(wlf + 0.5f);
	if (wl & 1 == 0) {
		wl--;
	}
	int wu = wl + 2;
    
	float mIdeal = (12.0f * Sigma * Sigma - nf * float(wl) * float(wl) - 4.0f * nf * float(wl) - 3.0f * nf) / (-4.0f * (float)wl - 4.0f);
	float mf = roundf(mIdeal);
	int m = float(mf + 0.5f);
    
	for (int i = 0; i < n; i++) {
		int ToSet = wu;
		if (i < m) {
			ToSet = wl;
		}
		Boxes[i] = ToSet;
	}
    
	BoxBlurApproximate(&Result, BitmapToBlur, (Boxes[0] - 1) / 2);
	BoxBlurApproximate(&TempBitmap, &Result, (Boxes[1] - 1) / 2);
	BoxBlurApproximate(&Result, &TempBitmap, (Boxes[2] - 1) / 2);
    
	return(Result);
}

bmp_info BlurBitmapExactGaussian(
bmp_info* BitmapToBlur,
void* ResultBitmapMem,
int width, int height,
int BlurRadius,
float* GaussianBox)
{
	Assert(width == BitmapToBlur->Width);
	Assert(height == BitmapToBlur->Height);
    
	bmp_info Result = AllocateBitmapInternal(
		BitmapToBlur->Width,
		BitmapToBlur->Height,
		ResultBitmapMem);
    
	int BlurDiam = 1 + BlurRadius + BlurRadius;
    
	bmp_info* From = BitmapToBlur;
	bmp_info* To = &Result;
    
	for (int Y = 0; Y < From->Height; Y++) {
		for (int X = 0; X < From->Width; X++) {
            
			u32* TargetPixel = (u32*)((u8*)To->Pixels + Y * To->Pitch + X * 4);
            
			v4 SumColor = {};
			for (int kY = Y - BlurRadius; kY <= Y + BlurRadius; kY++) {
				int targetY = Clamp(kY, 0, From->Height - 1);
				int inboxY = kY - (Y - BlurRadius);
				for (int kX = X - BlurRadius; kX <= X + BlurRadius; kX++) {
					int targetX = Clamp(kX, 0, From->Width - 1);
					int inboxX = kX - (X - BlurRadius);
                    
					u32* ScanPixel = (u32*)((u8*)From->Pixels + targetY * From->Pitch + targetX * 4);
                    
					v4 UnpackedColor = UnpackRGBA(*ScanPixel);
                    
					SumColor += UnpackedColor * GaussianBox[inboxY * BlurDiam + inboxX];
				}
			}
            
			*TargetPixel = PackRGBA(SumColor);
		}
	}
    
	return(Result);
}

// NOTE(Dima): Software rendering stuff
void RenderOneBitmapIntoAnother(
bmp_info* to, 
bmp_info* what,
int startX,
int startY,
v4 modulationColor) 
{
	float OneOver255 = 1.0f / 255.0f;
    
	int MaxToX = startX + what->Width;
	int MaxToY = startY + what->Height;
    
	Assert(MaxToX <= to->Width);
	Assert(MaxToY <= to->Height);
    
	u32 SrcX = 0;
	u32 SrcY = 0;
    
	for (int Y = startY; Y < MaxToY; Y++) {
		SrcY = Y - startY;
        
		for (int X = startX; X < MaxToX; X++) {
			SrcX = X - startX;
            
			u32* Out = (u32*)to->Pixels + Y * to->Width + X;
			v4 DstInitColor = UnpackRGBA(*Out);
            
            
			u32* From = (u32*)what->Pixels + SrcY * what->Width + SrcX;
            
			v4 FromColor = UnpackRGBA(*From);
            
			v4 ResultColor = FromColor * modulationColor;
            
			//NOTE(dima): Calculating blend alpha value
			float BlendAlpha = ResultColor.a;
            
			ResultColor.x = ResultColor.x + DstInitColor.x * (1.0f - BlendAlpha);
			ResultColor.y = ResultColor.y + DstInitColor.y * (1.0f - BlendAlpha);
			ResultColor.z = ResultColor.z + DstInitColor.z * (1.0f - BlendAlpha);
			ResultColor.a = ResultColor.a + DstInitColor.a - ResultColor.a * DstInitColor.a;
            
			u32 ColorValue = PackRGBA(ResultColor);
			*Out = ColorValue;
            
			SrcX++;
		}
        
		SrcY++;
	}
}


// NOTE(Dima): MESH AND SOUND STUFF

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

// NOTE(Dima): Asset stuff
INTERNAL_FUNCTION void BeginAsset(asset_system* System, u32 GroupID) {
	System->CurrentGroup = System->AssetGroups + GroupID;
	System->PrevAssetPointer = 0;
    
	game_asset_group* Group = System->CurrentGroup;
    
    // NOTE(Dima): Getting needed region
    int RegionIndex = Group->RegionsCount++;
    ASSERT(RegionIndex < ASSET_GROUP_REGIONS_COUNT);
    game_asset_group_region* Region = &Group->Regions[RegionIndex];
    
    Region->FirstAssetIndex = System->AssetCount;
    Region->AssetCount = 0;
}

INTERNAL_FUNCTION void EndAsset(asset_system* System) {
	Assert(System->CurrentGroup);
	game_asset_group* Group = System->CurrentGroup;
    
	System->CurrentGroup = 0;
	System->PrevAssetPointer = 0;
}

struct added_asset {
	game_asset* Asset;
	game_asset_source* Source;
	game_asset_freearea* Freearea;
	asset_header* FileHeader;
    u32 ID;
};

INTERNAL_FUNCTION added_asset AddAsset(asset_system* System, u32 AssetType) {
	added_asset Result = {};
    
    // NOTE(Dima): Getting needed group and region;
	Assert(System->CurrentGroup != 0);
	game_asset_group* Group = System->CurrentGroup;
    ASSERT(Group->RegionsCount > 0);
    
    game_asset_group_region* Region = &Group->Regions[Group->RegionsCount - 1];
	Region->AssetCount++;
    
    // NOTE(Dima): Setting needed data
	u32 AssetIndex = System->AssetCount;
	Result.Asset = System->Assets + AssetIndex;
	Result.Asset->Type = AssetType;
	Result.Source = System->AssetSources + AssetIndex;
	Result.Freearea = System->AssetFreeareas + AssetIndex;
	Result.FileHeader = System->FileHeaders + AssetIndex;
	Result.FileHeader->AssetType = AssetType;
    
	Result.Asset->ID = AssetIndex;
    Result.ID = AssetIndex;
	
    ++System->AssetCount;
    
	System->PrevAssetPointer = Result.Asset;
    
	return(Result);
}

INTERNAL_FUNCTION void AddFreeareaToAsset(asset_system* System, game_asset* Asset, void* Pointer) {
	game_asset_freearea* Free = System->AssetFreeareas + Asset->ID;
    
	int TargetFreeAreaIndex = Free->SetCount++;
	Assert(TargetFreeAreaIndex < FREEAREA_SLOTS_COUNT);
    
	Free->Pointers[TargetFreeAreaIndex] = Pointer;
}

inline game_asset_tag* FindTagInAsset(game_asset* Asset, u32 TagType) {
	game_asset_tag* Result = 0;
    
	for (int TagIndex = 0;
         TagIndex < Asset->TagCount;
         TagIndex++)
	{
		game_asset_tag* Tag = Asset->Tags + TagIndex;
		if (Tag->Type == TagType) {
			Result = Tag;
			break;
		}
	}
    
	return(Result);
}

INTERNAL_FUNCTION game_asset_tag* AddTag(asset_system* System, u32 TagType) {
	game_asset_tag* Result = 0;
    
	if (System->PrevAssetPointer) {
		/*
  NOTE(dima): First we should check if tag with
  the same type alredy exist.. Just for sure...
  */
		Result = FindTagInAsset(System->PrevAssetPointer, TagType);
        
		if (!Result) {
			if (System->PrevAssetPointer->TagCount < MAX_TAGS_PER_ASSET - 1) {
				//NOTE(dima): Getting tag and incrementing tag count
				Result = System->PrevAssetPointer->Tags + System->PrevAssetPointer->TagCount++;
				Result->Type = TagType;
			}
		}
	}
    
	return(Result);
}

INTERNAL_FUNCTION void AddFloatTag(asset_system* System, u32 TagType, float TagValue) {
	game_asset_tag* Tag = AddTag(System, TagType);
    
	if (Tag) {
		Tag->Value_Float = TagValue;
	}
}

INTERNAL_FUNCTION void AddIntTag(asset_system* System, u32 TagType, int TagValue) {
	game_asset_tag* Tag = AddTag(System, TagType);
    
	if (Tag) {
		Tag->Value_Int = TagValue;
	}
}

INTERNAL_FUNCTION void AddEmptyTag(asset_system* System, u32 TagType) {
	game_asset_tag* Tag = AddTag(System, TagType);
    
	if (Tag) {
		Tag->Value_Int = 1;
	}
}

INTERNAL_FUNCTION added_asset AddBitmapAsset(asset_system* System, char* Path, u32 BitmapLoadFlags = 0) {
	added_asset Added = AddAsset(System, AssetType_Bitmap);
    
    // NOTE(Dima): Setting source
	game_asset_source* Source = Added.Source;
    
	Source->BitmapSource.Path = Path;
	Source->BitmapSource.BitmapInfo = 0;
    Source->BitmapSource.LoadFlags = BitmapLoadFlags;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddBitmapAssetManual(asset_system* System, 
                                                   bmp_info* Bitmap, 
                                                   u32 BitmapLoadFlags = 0) 
{
	added_asset Added = AddAsset(System, AssetType_Bitmap);
    
	asset_header* FileHeader = Added.FileHeader;
    
    // NOTE(Dima): Setting source
	game_asset_source* Source = Added.Source;
	Source->BitmapSource.BitmapInfo = Bitmap;
    Source->BitmapSource.Path = 0;
    Source->BitmapSource.LoadFlags = BitmapLoadFlags;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddIconAsset(asset_system* System,
                                           char* Path)
{
    added_asset Result = AddBitmapAsset(System, Path, BitmapLoad_BakeIcon);
    
    return(Result);
}

INTERNAL_FUNCTION added_asset AddSoundAsset(asset_system* System, 
                                            char* Path) 
{
	added_asset Added = AddAsset(System, AssetType_Sound);
    
    // NOTE(Dima): Setting source
	game_asset_source* Source = Added.Source;
	Source->SoundSource.Path = Path;
    Source->SoundSource.Sound = 0;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddSoundAssetManual(asset_system* System, 
                                                  sound_info* Sound)
{
    added_asset Added = AddAsset(System, AssetType_Sound);
    
    // NOTE(Dima): Setting source
	game_asset_source* Source = Added.Source;
	Source->SoundSource.Sound = Sound;
    Source->SoundSource.Path = 0;
    
	return(Added);
}


INTERNAL_FUNCTION added_asset AddMeshAsset(asset_system* System, 
                                           mesh_info* Mesh) 
{
	added_asset Added = AddAsset(System, AssetType_Mesh);
    
	asset_header* FileHeader = Added.FileHeader;
    game_asset_source* Source = Added.Source;
	
    // NOTE(Dima): Setting source
    Source->MeshSource.MeshInfo = Mesh;
	
    // NOTE(Dima): Setting file header
	u32 VertexTypeSize = 0;
	if (Mesh->MeshType == Mesh_Simple) {
		FileHeader->Mesh.MeshType = Mesh_Simple;
		VertexTypeSize = sizeof(vertex_info);
	}
	else if(Mesh->MeshType == Mesh_Skinned) {
		FileHeader->Mesh.MeshType = Mesh_Skinned;
		VertexTypeSize = sizeof(vertex_skinned_info);
	}
    
	FileHeader->Mesh.VertexTypeSize = VertexTypeSize;
    FileHeader->Mesh.IndicesCount = sizeof(u32);
	FileHeader->Mesh.IndicesCount = Mesh->IndicesCount;
	FileHeader->Mesh.VerticesCount = Mesh->VerticesCount;
    
    FileHeader->Mesh.DataVerticesSize = FileHeader->Mesh.VertexTypeSize * Mesh->VerticesCount;
    FileHeader->Mesh.DataIndicesSize = sizeof(u32) * Mesh->IndicesCount;
    
	FileHeader->Mesh.DataOffsetToVertices = 0;
	FileHeader->Mesh.DataOffsetToIndices = FileHeader->Mesh.DataVerticesSize;
    
	AddFreeareaToAsset(System, Added.Asset, Mesh->Indices);
	AddFreeareaToAsset(System, Added.Asset, Mesh->Vertices);
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddFontAsset(
asset_system* System,
tool_font_info* FontInfo)
{
	added_asset Added = AddAsset(System, AssetType_Font);
    
	game_asset_source* Source = Added.Source;
	asset_header* Header = Added.FileHeader;
	
    Source->FontSource.FontInfo = FontInfo;
	Added.Asset->Font = FontInfo;
    
	Header->Font.AscenderHeight = FontInfo->AscenderHeight;
	Header->Font.DescenderHeight = FontInfo->DescenderHeight;
	Header->Font.LineGap = FontInfo->LineGap;
	Header->Font.GlyphCount = FontInfo->GlyphCount;
    
    if(FontInfo->GlyphCount){
        AddFreeareaToAsset(System, Added.Asset, FontInfo->KerningPairs);
    }
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddGlyphAssetInternal(
asset_system* System,
tool_glyph_info* GlyphInfo, 
u32 BitmapID)
{
    added_asset Added = AddAsset(System, AssetType_Glyph);
    
	game_asset_source* Source = Added.Source;
	Source->GlyphSource.Glyph = GlyphInfo;
    
	asset_header* Header = Added.FileHeader;
	Header->Glyph.Codepoint = GlyphInfo->Codepoint;
	Header->Glyph.BitmapWidth = GlyphInfo->Bitmap.Width;
	Header->Glyph.BitmapHeight = GlyphInfo->Bitmap.Height;
	Header->Glyph.BitmapWidthOverHeight = 
        (float)GlyphInfo->Bitmap.Width /
        (float)GlyphInfo->Bitmap.Height;
    Header->Glyph.XOffset = GlyphInfo->XOffset;
	Header->Glyph.YOffset = GlyphInfo->YOffset;
	Header->Glyph.Advance = GlyphInfo->Advance;
	Header->Glyph.LeftBearingX = GlyphInfo->LeftBearingX;
    Header->Glyph.BitmapID = BitmapID;
    
	return(Added);
}

INTERNAL_FUNCTION added_asset AddGlyphAsset(asset_system* System,
                                            tool_glyph_info* Glyph)
{
    added_asset BmpAsset = AddBitmapAssetManual(System, &Glyph->Bitmap, BitmapLoad_BakeIcon);
    added_asset Result = AddGlyphAssetInternal(System, Glyph, BmpAsset.ID);
    
    return(Result);
}

INTERNAL_FUNCTION added_asset AddBitmapArray(asset_system* System, 
                                             u32 FirstBitmapID, 
                                             int Count)
{
    added_asset Added = AddAsset(System, AssetType_BitmapArray);
    
	asset_header* Header = Added.FileHeader;
    
    Header->BmpArray.Count = Count;
    Header->BmpArray.FirstBmpID = FirstBitmapID;
    
    return(Added);
}

void InitAssetFile(asset_system* Assets) {
	//NOTE(dima): Reserving first asset to make it NULL asset
	Assets->AssetCount = 1;
	Assets->PrevAssetPointer = 0;
    
	//NOTE(dima): Clearing asset groups
	for (int AssetGroupIndex = 0;
         AssetGroupIndex < GameAsset_Count;
         AssetGroupIndex++)
	{
		game_asset_group* Group = Assets->AssetGroups + AssetGroupIndex;
        
        for(int i = 0; i < ASSET_GROUP_REGIONS_COUNT; i++)
        {
            Group->Regions[i].FirstAssetIndex = 0;
            Group->Regions[i].AssetCount = 0;
        }
	}
    
	//NOTE(dima): Clearing free areas
	for (int FreeAreaIndex = 0;
         FreeAreaIndex < TEMP_STORED_ASSET_COUNT;
         FreeAreaIndex++) 
	{
		game_asset_freearea* Free = Assets->AssetFreeareas + FreeAreaIndex;
        
		*Free = {};
        
		Free->SetCount = 0;
		for (int PointerIndex = 0; PointerIndex < FREEAREA_SLOTS_COUNT; PointerIndex++) {
			Free->Pointers[PointerIndex] = 0;
		}
	}
}


enum load_font_flags{
    LoadFont_BakeShadow = 1,
    LoadFont_BakeBlur = 2,
};

void LoadFontAddCodepoint(
tool_font_info* FontInfo, 
stbtt_fontinfo* STBFont,
int Codepoint,
int* AtlasWidth, 
int* AtlasHeight,
float FontScale,
u32 Flags,
int BlurRadius,
float* GaussianBox)
{
    int glyphIndex = FontInfo->GlyphCount++;
    tool_glyph_info* glyph = &FontInfo->Glyphs[glyphIndex];
    
    FontInfo->Codepoint2Glyph[Codepoint] = glyphIndex;
    
    int CharWidth;
	int CharHeight;
	int XOffset;
	int YOffset;
	int Advance;
	int LeftBearingX;
    
    u8* Bitmap = stbtt_GetCodepointBitmap(
        STBFont,
        FontScale, FontScale,
        Codepoint,
        &CharWidth, &CharHeight,
        &XOffset, &YOffset);
    
    if(CharWidth > 9999){
        CharWidth = 0;
    }
    if(CharHeight > 9999){
        CharHeight = 0;
    }
    
    int CharBorder = 3;
    int ShadowOffset = 0;
    
    if(Flags & LoadFont_BakeShadow){
        ShadowOffset = 2;
    }
    
    int CharBmpWidth= CharBorder * 2 + CharWidth;
    int CharBmpHeight = CharBorder * 2 + CharHeight;
    
    stbtt_GetCodepointHMetrics(STBFont, Codepoint, &Advance, &LeftBearingX);
    
    glyph->Width = CharBmpWidth + ShadowOffset;
	glyph->Height = CharBmpHeight + ShadowOffset;
	glyph->Bitmap = AllocateBitmap(glyph->Width, glyph->Height);
	glyph->Advance = Advance * FontScale;
	glyph->LeftBearingX = LeftBearingX * FontScale;
	glyph->XOffset = XOffset - CharBorder;
	glyph->YOffset = YOffset - CharBorder;
	glyph->Codepoint = Codepoint;
    
    *AtlasWidth += glyph->Width;
	*AtlasHeight = Max(*AtlasHeight, glyph->Height);
    
    //NOTE(dima): Clearing the image bytes
	u32* Pixel = (u32*)glyph->Bitmap.Pixels;
	for (int Y = 0; Y < glyph->Height; Y++) {
		for (int X = 0; X < glyph->Width; X++) {
			*Pixel++ = 0;
		}
	}
    
    //NOTE(dima): Forming char bitmap
	float OneOver255 = 1.0f / 255.0f;
    
    bmp_info CharBitmap= AllocateBitmap(CharWidth, CharHeight);
	
	for (int j = 0; j < CharHeight; j++) {
		for (int i = 0; i < CharWidth; i++) {
			u8 Grayscale = *((u8*)Bitmap + j * CharWidth + i);
			float Grayscale01 = (float)Grayscale * OneOver255;
            
			v4 resColor = V4(1.0f, 1.0f, 1.0f, Grayscale01);
            
			/*Alpha premultiplication*/
			resColor.r *= resColor.a;
			resColor.g *= resColor.a;
			resColor.b *= resColor.a;
            
			u32 ColorValue = PackRGBA(resColor);
			u32* TargetPixel = (u32*)((u8*)CharBitmap.Pixels + j* CharBitmap.Pitch + i * 4);
			*TargetPixel = ColorValue;
		}
	}
    
    //NOTE(dima): Render blur if needed
	if (Flags & LoadFont_BakeBlur) {
		bmp_info ToBlur = AllocateBitmap(
            2 * CharBorder + CharWidth,
            2 * CharBorder + CharHeight);
        
		bmp_info BlurredResult = AllocateBitmap(
            2 * CharBorder + CharWidth,
            2 * CharBorder + CharHeight);
        
		bmp_info TempBitmap = AllocateBitmap(
            2 * CharBorder + CharWidth,
            2 * CharBorder + CharHeight);
        
		RenderOneBitmapIntoAnother(
            &ToBlur,
            &CharBitmap,
            CharBorder,
            CharBorder,
            V4(1.0f, 1.0f, 1.0f, 1.0f));
        
#if 1
		BlurBitmapExactGaussian(
            &ToBlur,
            BlurredResult.Pixels,
            ToBlur.Width,
            ToBlur.Height,
            BlurRadius,
            GaussianBox);
        
        
		for (int Y = 0; Y < ToBlur.Height; Y++) {
			for (int X = 0; X < ToBlur.Width; X++) {
				u32* FromPix = (u32*)BlurredResult.Pixels + Y * BlurredResult.Width + X;
				u32* ToPix = (u32*)ToBlur.Pixels + Y * ToBlur.Width + X;
                
				v4 FromColor = UnpackRGBA(*FromPix);
                
				v4 resColor = FromColor;
				if (resColor.a < 0.15f) {
                    resColor.a = 0.0f;
				}
                else{
                    resColor.a = 1.0f;
                }
                
				*ToPix = PackRGBA(resColor);
			}
		}
        
		BlurBitmapExactGaussian(
            &ToBlur,
            BlurredResult.Pixels,
            ToBlur.Width,
            ToBlur.Height,
            BlurRadius,
            GaussianBox);
        
#else
		BlurBitmapApproximateGaussian(
            &ToBlur,
            BlurredResult.Pixels,
            TempBitmap.Pixels,
            ToBlur.Width,
            ToBlur.Height,
            BlurRadius);
		for (int Y = 0; Y < ToBlur.Height; Y++) {
			for (int X = 0; X < ToBlur.Width; X++) {
				u32* FromPix = (u32*)BlurredResult.Pixels + Y * BlurredResult.Width + X;
				u32* ToPix = (u32*)ToBlur.Pixels + Y * ToBlur.Width + X;
				v4 FromColor = UnpackRGBA(*FromPix);
				v4 resColor = FromColor;
				if (resColor.a > 0.05f) {
					resColor.a = 1.0f;
				}
				*ToPix = PackRGBA(resColor);
			}
		}
		BlurBitmapApproximateGaussian(
            &ToBlur,
            BlurredResult.Pixels,
            TempBitmap.Pixels,
            ToBlur.Width,
            ToBlur.Height,
            BlurRadius);
#endif
        
		RenderOneBitmapIntoAnother(
            &glyph->Bitmap,
            &BlurredResult,
            0, 0,
            V4(0.0f, 0.0f, 0.0f, 1.0f));
        
		DeallocateBitmap(&TempBitmap);
		DeallocateBitmap(&ToBlur);
		DeallocateBitmap(&BlurredResult);
	}
    
    if(Flags & LoadFont_BakeShadow){
        
        RenderOneBitmapIntoAnother(
            &glyph->Bitmap,
            &CharBitmap,
            CharBorder + ShadowOffset,
            CharBorder + ShadowOffset,
            V4(0.0f, 0.0f, 0.0f, 1.0f));
        
    }
    
    RenderOneBitmapIntoAnother(
        &glyph->Bitmap,
        &CharBitmap,
        CharBorder,
        CharBorder,
        V4(1.0f, 1.0f, 1.0f, 1.0f));
    
    DeallocateBitmap(&CharBitmap);
    stbtt_FreeBitmap(Bitmap, 0);
}

tool_font_info LoadFont(char* FilePath, float height, u32 Flags){
    tool_font_info res = {};
    
    stbtt_fontinfo STBFont;
    
    data_buffer TTFBuffer = ReadFileToDataBuffer(FilePath);
    stbtt_InitFont(&STBFont, TTFBuffer.Data, 
                   stbtt_GetFontOffsetForIndex(TTFBuffer.Data, 0));
    
    float Scale = stbtt_ScaleForPixelHeight(&STBFont, height);
    
    int AscenderHeight;
	int DescenderHeight;
	int LineGap;
    
    stbtt_GetFontVMetrics(
        &STBFont,
        &AscenderHeight,
        &DescenderHeight,
        &LineGap);
    
    res.AscenderHeight = (float)AscenderHeight * Scale;
	res.DescenderHeight = (float)DescenderHeight * Scale;
	res.LineGap = (float)LineGap * Scale;
    
    int AtlasWidth = 0;
    int AtlasHeight = 0;
    
    //NOTE(dima): This is for blurring
	int BlurRadius = 2;
	float GaussianBox[256];
	if (Flags & LoadFont_BakeBlur) {
        
		u32 GaussianBoxCompCount = Calcualte2DGaussianBoxComponentsCount(BlurRadius);
		Calculate2DGaussianBox(GaussianBox, BlurRadius);
	}
    
    for(int Codepoint = ' ';
        Codepoint <= '~';
        Codepoint++)
    {
        LoadFontAddCodepoint(
            &res, 
            &STBFont,
            Codepoint,
            &AtlasWidth,
            &AtlasHeight,
            Scale,
            Flags,
            BlurRadius,
            GaussianBox);
    }
    
    if(res.GlyphCount){
        //NOTE(dima): Processing kerning
        res.KerningPairs = (float*)malloc(sizeof(float) * res.GlyphCount * res.GlyphCount);
        
        for (int firstIndex = 0; firstIndex < res.GlyphCount; firstIndex++) {
            for (int secondIndex = 0; secondIndex < res.GlyphCount; secondIndex++) {
                u32 KerningIndex = secondIndex * res.GlyphCount + firstIndex;
                
                int FirstCodepoint = res.Glyphs[firstIndex].Codepoint;
                int SecondCodepoint = res.Glyphs[secondIndex].Codepoint;
                
                int Kern = stbtt_GetGlyphKernAdvance(&STBFont, FirstCodepoint, SecondCodepoint);
                
                res.KerningPairs[KerningIndex] = (float)Kern * Scale;
            }
        }
    }
    
    FreeDataBuffer(&TTFBuffer);
    
    return(res);
}


void WriteAssetFile(asset_system* Assets, char* FileName) {
	FILE* fp = fopen(FileName, "wb");
    
	u32 AssetsLinesOffsetsCount = Assets->AssetCount - 1;
	u32 AssetsLinesOffsetsSize = sizeof(u32) * AssetsLinesOffsetsCount;
	u32* AssetsLinesOffsets = (u32*)malloc(AssetsLinesOffsetsSize);
    
	u32 AssetFileBytesWritten = 0;
	u32 AssetLinesBytesWritten = 0;
	if (fp) {
        
		//NOTE(dima): Writing asset file header
		asset_file_header FileHeader = {};
        
		FileHeader.Version = GetVersionInt(ASSET_FILE_VERSION_MAJOR,
                                           ASSET_FILE_VERSION_MINOR);
		FileHeader.GroupsCount = GameAsset_Count;
		FileHeader.EffectiveAssetsCount = Assets->AssetCount - 1;
        
		FileHeader.FileHeader[0] = 'J';
		FileHeader.FileHeader[1] = 'A';
		FileHeader.FileHeader[2] = 'S';
		FileHeader.FileHeader[3] = 'S';
        
		size_t HeaderBytesWritten = fwrite(&FileHeader, sizeof(asset_file_header), 1, fp);
        AssetFileBytesWritten += sizeof(asset_file_header);
        
        //NOTE(dima): Writing asset groups after asset file header
        for (int GroupIndex = 0;
             GroupIndex < FileHeader.GroupsCount;
             GroupIndex++)
        {
            game_asset_group* Src = &Assets->AssetGroups[GroupIndex];
            asset_file_group Group;
            
            Group.Count = Src->RegionsCount;
            
            // NOTE(Dima): Copy all groups and regions
            int i;
            for(i = 0; i < Src->RegionsCount; i++)
            {
                Group.Regions[i].FirstAssetIndex = Src->Regions[i].FirstAssetIndex;
                Group.Regions[i].AssetCount = Src->Regions[i].AssetCount;
            }
            // NOTE(Dima): Set unnesessary to NULL
            for(i; i < ASSET_GROUP_REGIONS_COUNT; i++){
                Group.Regions[i].FirstAssetIndex = 0;
                Group.Regions[i].AssetCount = 0;
            }
            
            fwrite(&Group, sizeof(asset_file_group), 1, fp);
            AssetFileBytesWritten += sizeof(asset_file_group);
        }
        
        for (int AssetIndex = 1;
             AssetIndex < Assets->AssetCount;
             AssetIndex++)
        {
            //NOTE(dima): Setting asset line offset
            AssetsLinesOffsets[AssetIndex - 1] = ftell(fp);
            
            game_asset* Asset = Assets->Assets + AssetIndex;
            game_asset_source* Source = Assets->AssetSources + AssetIndex;
            game_asset_freearea* Free = Assets->AssetFreeareas + AssetIndex;
            asset_header* Header = Assets->FileHeaders + AssetIndex;
            
            u32 HeaderByteSize = sizeof(asset_header);
            u32 TagsByteSize = MAX_TAGS_PER_ASSET * sizeof(asset_tag_header);
            u32 DataByteSize = 0;
            
            /*
            NOTE(dima): Loading assets and setting assets
            headers data byte size.
            */
            switch (Asset->Type) {
                case AssetType_Bitmap: {
                    b32 BitmapAllocatedHere = 0;
                    if (!Source->BitmapSource.BitmapInfo) 
                    {
                        Asset->Bitmap = (bmp_info*)malloc(sizeof(bmp_info));
                        *Asset->Bitmap = LoadBMP(Source->BitmapSource.Path);
                        
                        BitmapAllocatedHere = 1;
                    }
                    else {
                        Asset->Bitmap = Source->BitmapSource.BitmapInfo;
                    }
                    AddFreeareaToAsset(Assets, Asset, Asset->Bitmap->Pixels);
                    
                    if (BitmapAllocatedHere) {
                        AddFreeareaToAsset(Assets, Asset, Asset->Bitmap);
                    }
                    
                    //NOTE(dima): Setting asset header
                    Header->Bitmap.Width = Asset->Bitmap->Width;
                    Header->Bitmap.Height = Asset->Bitmap->Height;
                    Header->Bitmap.BakeToAtlas = ((Source->BitmapSource.LoadFlags & 
                                                   BitmapLoad_BakeIcon) != 0);
                    
                    //NOTE(dima): Set data size
                    DataByteSize = Asset->Bitmap->Width * Asset->Bitmap->Height * 4;
                }break;
                
                case AssetType_Sound:{
                    b32 SoundAllocatedHere = 0;
                    if(!Source->SoundSource.Sound){
                        Asset->Sound = (sound_info*)malloc(sizeof(sound_info));
                        *Asset->Sound = LoadSound(Source->SoundSource.Path);
                        
                        SoundAllocatedHere = 1;
                    }
                    else{
                        Asset->Sound = Source->SoundSource.Sound;
                    }
                    AddFreeareaToAsset(Assets, Asset, Asset->Sound->Samples[0]);
                    
                    if(SoundAllocatedHere){
                        AddFreeareaToAsset(Assets, Asset, Asset->Sound);
                    }
                    
                    // NOTE(Dima): Setting asset header
                    Header->Sound.SampleCount = Asset->Sound->SampleCount;
                    Header->Sound.SamplesPerSec = Asset->Sound->SamplesPerSec;
                    Header->Sound.Channels = Asset->Sound->Channels;
                    
                    // NOTE(Dima): Setting data size
                    DataByteSize = Asset->Sound->SampleCount * Asset->Sound->Channels * sizeof(i16);
                }break;
                
                case AssetType_Font: {
                    Asset->Font = Source->FontSource.FontInfo;
                    
                    u32 SizeOfMapping = sizeof(int) * FONT_INFO_MAX_GLYPH_COUNT;
                    u32 SizeOfIDs = sizeof(u32) * Asset->Font->GlyphCount;
                    u32 SizeOfKerning = sizeof(float) * 
                        Asset->Font->GlyphCount * 
                        Asset->Font->GlyphCount;
                    
                    Header->Font.MappingSize = SizeOfMapping;
                    Header->Font.KerningSize = SizeOfKerning;
                    Header->Font.IDsSize = SizeOfIDs;
                    
                    Header->Font.DataOffsetToMapping = 0;
                    Header->Font.DataOffsetToKerning = SizeOfMapping;
                    Header->Font.DataOffsetToIDs = SizeOfMapping + SizeOfKerning;
                    
                    DataByteSize = SizeOfMapping + SizeOfKerning + SizeOfIDs;
                }break;
                
                case AssetType_Glyph: {
                    // NOTE(Dima): Nothing to write and set
                }break;
                
                case AssetType_Mesh: {
                    Asset->Mesh = Source->MeshSource.MeshInfo;
                    
                    DataByteSize = 
                        Header->Mesh.DataVerticesSize + 
                        Header->Mesh.DataIndicesSize;
                }break;
            }
            
            //NOTE(dima): Forming header
            Header->LineDataOffset = GetLineOffsetForData();
            Header->Pitch = HeaderByteSize + DataByteSize;
            Header->TagCount = Asset->TagCount;
            Header->AssetType = Asset->Type;
            Header->TotalDataSize = DataByteSize;
            Header->TotalTagsSize = TagsByteSize;
            
            // NOTE(Dima): Forming tags in header
            for (int AssetTagIndex = 0;
                 AssetTagIndex < MAX_TAGS_PER_ASSET;
                 AssetTagIndex++)
            {
                game_asset_tag* From = Asset->Tags + AssetTagIndex;
                asset_tag_header* To = Header->Tags + AssetTagIndex;
                
                To->Type = From->Type;
                To->Value_Float = From->Value_Float;
            }
            
            // NOTE(Dima): Writing asset header
            fwrite(Header, sizeof(asset_header), 1, fp);
            
            //NOTE(dima): Writing asset data
            switch (Asset->Type) {
                case AssetType_Bitmap: {
                    //NOTE(dima): Writing bitmap pixel data
                    fwrite(Asset->Bitmap->Pixels, DataByteSize, 1, fp);
                }break;
                
                case AssetType_Sound:{
                    fwrite(Asset->Sound->Samples, DataByteSize, 1, fp);
                }break;
                
                case AssetType_Font: {
                    // IMPORTANT(Dima): Order is important
                    
                    // NOTE(Dima): FIRST - WRITING MAPPING
                    fwrite(
						Asset->Font->Codepoint2Glyph,
						Header->Font.MappingSize,
						1, fp);
                    
                    //NOTE(dima): SECOND - WRITING KERNING
                    fwrite(
                        Asset->Font->KerningPairs,
                        Header->Font.KerningSize,
                        1, fp);
                    
                    //NOTE(dima): THIRD - Write glyph IDs
                    fwrite(
                        Asset->Font->GlyphIDs,
                        Header->Font.IDsSize,
                        1, fp);
                    
                }break;
                
                case AssetType_Glyph: {
                    //NOTE(dima): Nothing to write
                }break;
                
                case AssetType_Mesh: {
                    //NOTE(dima): Writing vertices
                    fwrite(Asset->Mesh->Vertices,
                           Header->Mesh.DataVerticesSize, 
                           1, fp);
                    
                    //NOTE(dima): Writing indices
                    fwrite(
                        Asset->Mesh->Indices,
                        Header->Mesh.DataIndicesSize,
                        1, fp);
                }break;
            }
            
            //NOTE(dima): Freeing freareas
            for (int FreeIndex = 0; FreeIndex < Free->SetCount; FreeIndex++) {
                if(Free->Pointers[FreeIndex]){
                    free(Free->Pointers[FreeIndex]);
                    Free->Pointers[FreeIndex] = 0;
                }
            }
            
            //NOTE(dima): Incrementing file written data size
            AssetFileBytesWritten += Header->Pitch;
            AssetLinesBytesWritten += Header->Pitch;
        }
        
        fclose(fp);
    }
    else {
        INVALID_CODE_PATH;
    }
    
    
    //NOTE(dima): Reading file contents
    void* FileData = 0;
    fp = fopen(FileName, "rb");
    if (fp) {
        //NOTE(dima): Getting file size
        fseek(fp, 0, SEEK_END);
        u32 FileSize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
        ASSERT(FileSize == AssetFileBytesWritten);
        
        //NOTE(dima): Reading file contents
        FileData = malloc(FileSize);
        fread(FileData, FileSize, 1, fp);
        
        fclose(fp);
    }
    else {
        INVALID_CODE_PATH;
    }
    
    //NOTE(dima): Incrementing asset lines offsets by size of asset lines offsets array
    for (int LineIndex = 0;
         LineIndex < AssetsLinesOffsetsCount;
         LineIndex++)
    {
        AssetsLinesOffsets[LineIndex] += AssetsLinesOffsetsSize;
    }
    
    //NOTE(dima): Inserting asset lines offsets after groups
    fp = fopen(FileName, "wb");
    if (fp) {
        asset_file_header* Header = (asset_file_header*)FileData;
        
        u32 GroupsByteSize = Header->GroupsCount * sizeof(asset_file_group);
        u32 LinesOffsetsSize = Header->EffectiveAssetsCount * sizeof(u32);
        u32 AssetsLinesByteSize = AssetLinesBytesWritten;
        
        u32 GroupsByteOffset = sizeof(asset_file_header);
        u32 LinesOffsetsByteOffset = GroupsByteOffset + GroupsByteSize;
        u32 AssetLinesByteOffset = LinesOffsetsByteOffset + LinesOffsetsSize;
        
        Header->GroupsByteOffset = GroupsByteOffset;
        Header->LinesOffsetsByteOffset = LinesOffsetsByteOffset;
        Header->AssetLinesByteOffset = AssetLinesByteOffset;
        
        //NOTE(dima): Rewriting header
        fwrite(Header, sizeof(asset_file_header), 1, fp);
        
        //NOTE(dima): Rewriting groups
        ASSERT(GroupsByteOffset == ftell(fp));
        fwrite((u8*)FileData + GroupsByteOffset, GroupsByteSize, 1, fp);
        
        //NOTE(dima): Writing asset lines offsets
        ASSERT(LinesOffsetsByteOffset == ftell(fp));
        fwrite(AssetsLinesOffsets, AssetsLinesOffsetsSize, 1, fp);
        
        //NOTE(dima): Rewriting asset data lines
        ASSERT(AssetLinesByteOffset == ftell(fp));
        fwrite(
            (u8*)FileData + GroupsByteOffset + GroupsByteSize, 
            AssetLinesBytesWritten, 1, fp);
    }
    else {
        INVALID_CODE_PATH;
    }
    
    if (FileData) {
        free(FileData);
    }
    
    free(AssetsLinesOffsets);
    
    printf("File %s has been written...\n", FileName);
}

INTERNAL_FUNCTION void WriteFontsChunk(tool_font_info** Fonts,
                                       u32* FontsGroups,
                                       game_asset_tag_hub* FontsTags,
                                       int FontsCount, 
                                       int ChunkIndex)
{
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    // NOTE(Dima): Adding glyphs
    BeginAsset(System, GameAsset_Type_Glyph);
    for (int FontIndex = 0;
         FontIndex < FontsCount;
         FontIndex++)
    {
        tool_font_info* Font = Fonts[FontIndex];
        
        for (int GlyphIndex = 0;
             GlyphIndex < Font->GlyphCount;
             GlyphIndex++)
        {
            added_asset AddedGlyphAsset = AddGlyphAsset(System, &Font->Glyphs[GlyphIndex]);
            
            Font->GlyphIDs[GlyphIndex] = AddedGlyphAsset.ID;
        }
    }
    EndAsset(System);
    
    // NOTE(Dima): Adding fonts
    for (int FontIndex = 0;
         FontIndex < FontsCount;
         FontIndex++)
    {
        tool_font_info* Font = Fonts[FontIndex];
        game_asset_tag_hub* TagHub = FontsTags + FontIndex;
        
        BeginAsset(System, FontsGroups[FontIndex]);
        // NOTE(Dima): Adding font asset
        AddFontAsset(System, Font);
        // NOTE(Dima): Adding font tags
        for(int TagIndex = 0; 
            TagIndex < TagHub->TagCount;
            TagIndex++)
        {
            game_asset_tag* Tag = TagHub->Tags + TagIndex;
            
            switch(TagHub->TagValueTypes[TagIndex]){
                case GameAssetTagValue_Float:{
                    AddFloatTag(System, Tag->Type, Tag->Value_Float);
                }break;
                
                case GameAssetTagValue_Int:{
                    AddIntTag(System, Tag->Type, Tag->Value_Int);
                }break;
                
                case GameAssetTagValue_Empty:{
                    AddEmptyTag(System, Tag->Type);
                }break;
            }
        }
        EndAsset(System);
    }
    
    // NOTE(Dima): Writing file
    char OutFileName[256];
    stbsp_sprintf(OutFileName, "../Data/Fonts%d.ja", ChunkIndex);
    
    WriteAssetFile(System, OutFileName);
}

INTERNAL_FUNCTION void WriteFonts(){
    tool_font_info LibMono = LoadFont("../Data/Fonts/LiberationMono-Regular.ttf", 16.0f, LoadFont_BakeShadow);
    tool_font_info Lilita = LoadFont("../Data/Fonts/LilitaOne.ttf", 20.0f, LoadFont_BakeShadow);
    tool_font_info Inconsolata = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 16.0f, LoadFont_BakeBlur);
    tool_font_info PFDIN = LoadFont("../Data/Fonts/PFDinTextCondPro-Regular.ttf", 16.0f, LoadFont_BakeBlur);
    tool_font_info MollyJack = LoadFont("../Data/Fonts/MollyJack.otf", 40.0f, LoadFont_BakeBlur);
    
    tool_font_info* Fonts[] = {
        &LibMono,
        &Lilita,
        &Inconsolata,
        &PFDIN,
        &MollyJack,
    };
    
    u32 Groups[] = {
        GameAsset_LiberationMono,
        GameAsset_LilitaOne,
        GameAsset_Inconsolata,
        GameAsset_PFDIN,
        GameAsset_MollyJackFont,
    };
    
    game_asset_tag_hub Tags[] = {
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Bold),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
    };
    
    int FontCount = ArrayCount(Fonts);
    
    int ChunkIndex = 0;
    for(int FontIndex = 0;
        FontIndex < FontCount;
        FontIndex += ASSET_GROUP_REGIONS_COUNT,
        ChunkIndex++)
    {
        tool_font_info** CurFontsChunk = &Fonts[FontIndex];
        u32 * CurChunkGroups = Groups + FontIndex;
        game_asset_tag_hub* CurChunkTagHub = Tags + FontIndex;
        
        int CurChunkSize = Min(FontCount - FontIndex, ASSET_GROUP_REGIONS_COUNT);
        
        if(CurChunkSize){
            WriteFontsChunk(CurFontsChunk, 
                            CurChunkGroups,
                            CurChunkTagHub,
                            CurChunkSize, 
                            ChunkIndex);
        }
    }
}

INTERNAL_FUNCTION void WriteIcons(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    BeginAsset(System, GameAsset_CheckboxMark);
    AddIconAsset(System, "../Data/Icons/checkmark64.png");
    EndAsset(System);
    
    BeginAsset(System, GameAsset_ChamomileIcon);
    AddIconAsset(System, "../Data/Icons/chamomile.png");
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/Icons.ja");
}

INTERNAL_FUNCTION void WriteBitmapArray(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    std::string FolderPath("../Data/Images/");
    std::string ListFileName("ToLoadImages.txt");
    
    std::vector<std::string> BitmapPaths;
    
    // NOTE(Dima): Initializing bitmap paths
    Loaded_Strings BitmapNames = LoadStringListFromFile((char*)(FolderPath + ListFileName).c_str());
    int BitmapCount = BitmapNames.Count;
    for(int i = 0; i < BitmapCount; i++){
        BitmapPaths.push_back(FolderPath + std::string(BitmapNames.Strings[i]));
    }
    FreeStringList(&BitmapNames);
    
    // NOTE(Dima): Adding bitmap assets
    BeginAsset(System, GameAsset_Type_Bitmap);
    u32 FirstBitmapID = 0;
    for(int i = 0; i < BitmapPaths.size(); i++){
        added_asset Asset = AddBitmapAsset(System, (char*)BitmapPaths[i].c_str());
        
        if(i == 0){
            FirstBitmapID = Asset.ID;
        }
    }
    EndAsset(System);
    
    // NOTE(Dima): adding bitmap array
    BeginAsset(System, GameAsset_FadeoutBmps);
    AddBitmapArray(System, FirstBitmapID, BitmapCount);
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/BitmapsArray.ja");
}

INTERNAL_FUNCTION void WriteBitmaps(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    
    
    WriteAssetFile(System, "../Data/Bitmaps.ja");
}

INTERNAL_FUNCTION void WriteSounds(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    sound_info Sine = MakeSineSound256(44100 * 2, 44100);
    
    BeginAsset(System, GameAsset_SineTest);
    AddSoundAssetManual(System, &Sine);
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/Sounds.ja");
}

INTERNAL_FUNCTION void WriteMeshPrimitives(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    mesh_info Cube = MakeCube();
    mesh_info Plane = MakePlane();
    
    mesh_info SphereMeshSuperHig = MakeSphere(160, 80);
    mesh_info SphereMeshHig = MakeSphere(80, 40);
    mesh_info SphereMeshAvg = MakeSphere(40, 20);
    mesh_info SphereMeshLow = MakeSphere(20, 10);
    mesh_info SphereMeshSuperLow = MakeSphere(10, 5);
    
    mesh_info CylMeshSuperLow = MakeCylynder(2.0f, 0.5f, 6);
    mesh_info CylMeshLow = MakeCylynder(2.0f, 0.5f, 12);
    mesh_info CylMeshAvg = MakeCylynder(2.0f, 0.5f, 24);
    mesh_info CylMeshHig = MakeCylynder(2.0f, 0.5f, 48);
    mesh_info CylMeshSuperHig = MakeCylynder(2.0f, 0.5f, 96);
    
    BeginAsset(System, GameAsset_Cube);
    AddMeshAsset(System, &Cube);
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Plane);
    AddMeshAsset(System, &Plane);
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Sphere);
    AddMeshAsset(System, &SphereMeshSuperLow);
    AddFloatTag(System, AssetTag_LOD, 0.0f);
    AddMeshAsset(System, &SphereMeshLow);
    AddFloatTag(System, AssetTag_LOD, 0.25f);
    AddMeshAsset(System, &SphereMeshAvg);
    AddFloatTag(System, AssetTag_LOD, 0.5f);
    AddMeshAsset(System, &SphereMeshHig);
    AddFloatTag(System, AssetTag_LOD, 0.75f);
    AddMeshAsset(System, &SphereMeshSuperHig);
    AddFloatTag(System, AssetTag_LOD, 1.0f);
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Cylynder);
    AddMeshAsset(System, &CylMeshSuperLow);
    AddFloatTag(System, AssetTag_LOD, 0.0f);
    AddMeshAsset(System, &CylMeshLow);
    AddFloatTag(System, AssetTag_LOD, 0.25f);
    AddMeshAsset(System, &CylMeshAvg);
    AddFloatTag(System, AssetTag_LOD, 0.5f);
    AddMeshAsset(System, &CylMeshHig);
    AddFloatTag(System, AssetTag_LOD, 0.75f);
    AddMeshAsset(System, &CylMeshSuperHig);
    AddFloatTag(System, AssetTag_LOD, 1.0f);
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/MeshPrimitives.ja");
}

int main() {
    WriteBitmaps();
    WriteIcons();
    WriteMeshPrimitives();
    WriteFonts();
    WriteBitmapArray();
    WriteSounds();
    
    system("pause");
    return(0);
}