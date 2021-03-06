#include "tool_asset_build_loading.h"

// NOTE(Dima): For offsetof
#include <cstddef>

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"


#define STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_STATIC
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

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

tool_bmp_info AllocateBitmapInternal(u32 Width, u32 Height, void* pixelsData) {
	tool_bmp_info res = {};
    
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

void AllocateBitmapInternal(tool_bmp_info* Bmp, u32 Width, u32 Height, void* PixelsData){
    
    if(Bmp){
        *Bmp = AllocateBitmapInternal(Width, Height, PixelsData);
    }
    
}


tool_bmp_info AllocateBitmap(u32 Width, u32 Height) {
	u32 BitmapDataSize = Width * Height * 4;
	void* PixelsData = calloc(BitmapDataSize, 1);
    
	memset(PixelsData, 0, BitmapDataSize);
    
	tool_bmp_info res = AllocateBitmapInternal(Width, Height, PixelsData);
    
	return(res);
}

void CopyBitmapData(tool_bmp_info* Dst, tool_bmp_info* Src) {
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

void DeallocateBitmap(tool_bmp_info* Buffer) {
	if (Buffer->Pixels) {
		free(Buffer->Pixels);
	}
}

INTERNAL_FUNCTION tool_bmp_info LoadBMPInternal(unsigned char* Image,
                                                int Width,
                                                int Height)
{
    
    tool_bmp_info Result = {};
    
    int pixelsCount = Width * Height;
    int ImageSize = pixelsCount * 4;
    
    void* OurImageMem = malloc(ImageSize);
    Result = AllocateBitmapInternal(Width, Height, OurImageMem);
    
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
        
        _mm_storeu_si128((__m128i*)((u32*)Result.Pixels + PixelIndex), mmResult);
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
        
        *((u32*)Result.Pixels + PixelIndex) = PackedColor;
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
    
    return(Result);
}

tool_bmp_info LoadBMP(char* FilePath){
    int Width;
    int Height;
    int Channels;
    
    unsigned char* Image = stbi_load(
        FilePath,
        &Width,
        &Height,
        &Channels,
        STBI_rgb_alpha);
    
    Assert(Image);
    
    tool_bmp_info res = LoadBMPInternal(Image, Width, Height);
    
    stbi_image_free(Image);
    
    return(res);
}

tool_bmp_info LoadFromDataBMP(unsigned char* RawData, u32 RawDataSize){
    int Width;
    int Height;
    int Channels;
    
    unsigned char* Image = stbi_load_from_memory(RawData, 
                                                 RawDataSize, 
                                                 &Width,
                                                 &Height,
                                                 &Channels,
                                                 STBI_rgb_alpha);
    
    tool_bmp_info Result = LoadBMPInternal(Image, Width, Height);
    stbi_image_free(Image);
    
    return(Result);
}

tool_sound_info LoadSound(char* FilePath){
    tool_sound_info Result = {};
    
    
    
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
tool_bmp_info* To,
tool_bmp_info* From,
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

tool_bmp_info BlurBitmapApproximateGaussian(
tool_bmp_info* BitmapToBlur,
void* ResultBitmapMem,
void* TempBitmapMem,
int width, int height,
int BlurRadius)
{
	Assert(width == BitmapToBlur->Width);
	Assert(height == BitmapToBlur->Height);
    
	tool_bmp_info Result = AllocateBitmapInternal(
		BitmapToBlur->Width,
		BitmapToBlur->Height,
		ResultBitmapMem);
    
	tool_bmp_info TempBitmap = AllocateBitmapInternal(
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

tool_bmp_info BlurBitmapExactGaussian(
tool_bmp_info* BitmapToBlur,
void* ResultBitmapMem,
int width, int height,
int BlurRadius,
float* GaussianBox)
{
	Assert(width == BitmapToBlur->Width);
	Assert(height == BitmapToBlur->Height);
    
	tool_bmp_info Result = AllocateBitmapInternal(
		BitmapToBlur->Width,
		BitmapToBlur->Height,
		ResultBitmapMem);
    
	int BlurDiam = 1 + BlurRadius + BlurRadius;
    
	tool_bmp_info* From = BitmapToBlur;
	tool_bmp_info* To = &Result;
    
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
tool_bmp_info* to, 
tool_bmp_info* what,
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
tool_mesh_info MakeMesh(
std::vector<v3>& Positions,
std::vector<v2>& TexCoords,
std::vector<v3>& Normals,
std::vector<v3>& Tangents,
std::vector<u32> Indices,
std::vector<vertex_weights> Weights,
b32 CalculateNormals,
b32 CalculateTangents)
{
	tool_mesh_info Result = {};
    
    u32 VerticesCount = Positions.size();
    
    b32 HasSkinning = Weights.size() > 0;
    
    mesh_type_context TypeCtx = MeshSimpleType();
    
    if(HasSkinning){
        TypeCtx = MeshSkinnedType();
    }
    
    Result.TypeCtx = TypeCtx;
    Result.IndicesCount = Indices.size();
	Result.Indices = (u32*)malloc(Indices.size() * sizeof(u32));
	for (int IndexIndex = 0;
         IndexIndex < Indices.size();
         IndexIndex++)
	{
		Result.Indices[IndexIndex] = Indices[IndexIndex];
	}
	
    Result.VerticesCount = VerticesCount;
    size_t VertsSize = TypeCtx.VertexTypeSize * VerticesCount;
	Result.Vertices = malloc(VertsSize);
    memset(Result.Vertices, 0, VertsSize);
    
    ASSERT(Positions.size());
    
    if(TexCoords.size() == 0){
        TexCoords.insert(TexCoords.begin(), Positions.size(), V2(0.0f, 0.0f));
    }
    
    void* DstVerts = (void*)Result.Vertices;
    
#define SETM(index, member, type, value) SetVertsMemberData_##type(\
    DstVerts, TypeCtx.VertexTypeSize,\
    index, TypeCtx.Offset##member, value)
    
    for(int VertexIndex = 0;
        VertexIndex < VerticesCount;
        VertexIndex++)
    {
        SETM(VertexIndex, P, v3, Positions[VertexIndex]);
        SETM(VertexIndex, UV, v2, TexCoords[VertexIndex]);
        
        if(HasSkinning){
            // NOTE(Dima): Normalizing weights
            vertex_weights* CurWeights = &Weights[VertexIndex];
            
            int WalkCount = CurWeights->Weights.size();
            
#if 0            
            // NOTE(Dima): Calculating squared sums
            float SumOfSquaredWeights = 0.0f;
            for(int WeitEntryIndex = 0;
                WeitEntryIndex < WalkCount;
                WeitEntryIndex++)
            {
                vertex_weight* CurWeit = &CurWeights->Weights[WeitEntryIndex];
                
                SumOfSquaredWeights += CurWeit->Weight * CurWeit->Weight;
            }
            
            // NOTE(Dima): Actual normalizing
            float OneOverNorm = 1.0f / Sqrt(SumOfSquaredWeights);
            CurWeights->SumOfSquaredWeights = 0;
            
            for(int WeitEntryIndex = 0;
                WeitEntryIndex < WalkCount;
                WeitEntryIndex++)
            {
                vertex_weight* CurWeit = &CurWeights->Weights[WeitEntryIndex];
                
                CurWeit->Weight *= OneOverNorm;
                CurWeights->SumOfSquaredWeights += (CurWeit->Weight * CurWeit->Weight);
            }
#endif
            
            
            if(WalkCount > MAX_WEIGHTS_PER_VERTEX){
                // NOTE(Dima): Pick greatest 4 weights and put them in first 4 elements(unordered)
                // NOTE(Dima): Simple selection sort
                for(int i = 0; i < MAX_WEIGHTS_PER_VERTEX; i++){
                    
                    int MaxIndex = i + 1;
                    float MaxValue = CurWeights->Weights[MaxIndex].Weight;
                    
                    for(int j = i + 1; j < WalkCount; j++){
                        if(CurWeights->Weights[j].Weight > MaxValue){
                            MaxValue = CurWeights->Weights[j].Weight;
                            MaxIndex = j;
                        }
                    }
                    
                    if(CurWeights->Weights[MaxIndex].Weight > CurWeights->Weights[i].Weight){
                        std::swap(CurWeights->Weights[MaxIndex], CurWeights->Weights[i]);
                    }
                }
                
                WalkCount = MAX_WEIGHTS_PER_VERTEX;
            }
            
            // NOTE(Dima): Setting resulted values
            u32 ResultBoneIDs = 0;
            v4 ResultWeights = V4(0.0f, 0.0f, 0.0f, 0.0f);
            
            for(int WeightIndex = 0;
                WeightIndex < WalkCount;
                WeightIndex++)
            {
                vertex_weight W = CurWeights->Weights[WeightIndex];
                
                ResultBoneIDs |= (W.BoneID & 0xFF) << (WeightIndex * 8);
                ResultWeights.e[WeightIndex] = W.Weight;
            }
            
            SETM(VertexIndex, Weights, v4, ResultWeights);
            SETM(VertexIndex, BoneIDs, u32, ResultBoneIDs);
        }
    }
    
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
            SETM(Index0, T, v3, T);
            SETM(Index1, T, v3, T);
            SETM(Index2, T, v3, T);
        }
        else{
            //NOTE(dima): Just copy tangents if they exist
            SETM(Index0, T, v3, Tangents[Index0]);
            SETM(Index1, T, v3, Tangents[Index1]);
            SETM(Index2, T, v3, Tangents[Index2]);
        }
        
        //NOTE(dima): Normals calculation and setting
        if (CalculateNormals || (Normals.size() != VerticesCount)) {
            v3 TriNormal = Normalize(Cross(Edge2, Edge1));
            
            SETM(Index0, N, v3, TriNormal);
            SETM(Index1, N, v3, TriNormal);
            SETM(Index2, N, v3, TriNormal);
        }
        else{
            SETM(Index0, N, v3, Normals[Index0]);
            SETM(Index1, N, v3, Normals[Index1]);
            SETM(Index2, N, v3, Normals[Index2]);
        }
    }
    
    return(Result);
}

tool_mesh_info MakePlane(){
    tool_mesh_info Result = {};
    
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
    
    v2 T0 = V2(0.0f, 1.0f);
    v2 T1 = V2(1.0f, 1.0f);
    v2 T2 = V2(1.0f, 0.0f);
    v2 T3 = V2(0.0f, 0.0f);
    
    Positions.push_back(Ps[0]);
    Positions.push_back(Ps[1]);
    Positions.push_back(Ps[2]);
    Positions.push_back(Ps[3]);
    
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
                      Indices,
                      std::vector<vertex_weights>(),
                      JOY_FALSE, 
                      JOY_TRUE);
    
    return(Result);
}

INTERNAL_FUNCTION inline 
void PushSide(std::vector<v3>& Positions,
              std::vector<v2>& TexCoords,
              std::vector<v3>& Normals,
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
    
    v2 T0 = V2(0.0f, 1.0f);
    v2 T1 = V2(1.0f, 1.0f);
    v2 T2 = V2(1.0f, 0.0f);
    v2 T3 = V2(0.0f, 0.0f);
    
    Positions.push_back(Ps[i1]);
    Positions.push_back(Ps[i2]);
    Positions.push_back(Ps[i3]);
    Positions.push_back(Ps[i4]);
    
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

tool_mesh_info MakeCube(){
    tool_mesh_info Result = {};
    
    std::vector<v3> Positions;
    std::vector<v2> TexCoords;
    std::vector<v3> Normals;
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
             Normals, Indices,
             4, 5, 1, 0,
             Up, &VertAt);
    
    // NOTE(Dima): Pushing bottom
    PushSide(Positions, TexCoords,
             Normals, Indices,
             3, 2, 6, 7,
             Down, &VertAt);
    
    // NOTE(Dima): Pushing front
    PushSide(Positions, TexCoords,
             Normals, Indices,
             0, 1, 2, 3,
             Front, &VertAt);
    
    // NOTE(Dima): Pushing back
    PushSide(Positions, TexCoords,
             Normals, Indices,
             5, 4, 7, 6,
             Back, &VertAt);
    
    // NOTE(Dima): Pushing left
    PushSide(Positions, TexCoords,
             Normals, Indices,
             1, 5, 6, 2,
             Left, &VertAt);
    
    // NOTE(Dima): Pushing right
    PushSide(Positions, TexCoords,
             Normals, Indices,
             4, 0, 3, 7,
             Right, &VertAt);
    
    Result = MakeMesh(Positions, 
                      TexCoords, 
                      Normals, 
                      std::vector<v3>(), 
                      Indices,
                      std::vector<vertex_weights>(),
                      JOY_FALSE, 
                      JOY_TRUE);
    
    return(Result);
}

tool_mesh_info MakeSphere(int Segments, int Rings) {
    tool_mesh_info Result = {};
    
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
                      Indices,
                      std::vector<vertex_weights>(),
                      JOY_FALSE, 
                      JOY_TRUE);
    
    return(Result);
}

tool_mesh_info MakeCylynder(float Height, float Radius, int SidesCount) {
    tool_mesh_info Result = {};
    
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
                      Indices,
                      std::vector<vertex_weights>(),
                      JOY_FALSE, 
                      JOY_TRUE);
    
    return(Result);
}

tool_sound_info MakeSound(const std::vector<i16>& Samples,
                          int SamplesPerSec)
{
    tool_sound_info Result = {};
    
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

tool_sound_info MakeSineSound(const std::vector<int>& Frequencies, 
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
    
    tool_sound_info Result = MakeSound(Samples, SamplesPerSec);
    
    return(Result);
}

tool_sound_info MakeSineSound256(int SampleCount, int SamplesPerSec){
    std::vector<int> Freq{256};
    
    tool_sound_info Result = MakeSineSound(Freq, SampleCount, SamplesPerSec);
    
    return(Result);
}

INTERNAL_FUNCTION void LoadFontAddCodepoint(
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
    
    tool_bmp_info CharBitmap= AllocateBitmap(CharWidth, CharHeight);
    
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
        tool_bmp_info ToBlur = AllocateBitmap(
            2 * CharBorder + CharWidth,
            2 * CharBorder + CharHeight);
        
        tool_bmp_info BlurredResult = AllocateBitmap(
            2 * CharBorder + CharWidth,
            2 * CharBorder + CharHeight);
        
        tool_bmp_info TempBitmap = AllocateBitmap(
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
                if (resColor.a < 0.05f) {
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