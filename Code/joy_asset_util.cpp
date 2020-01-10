#include "joy_asset_util.h"
#include "joy_software_renderer.h"
#include "joy_defines.h"
#include "joy_render_blur.h"
#include "joy_simd.h"

#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_STATIC
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

#include <intrin.h>

Data_Buffer ReadFileToDataBuffer(char* fileName){
	Data_Buffer res = {};
    
	FILE* fp = fopen(fileName, "rb");
	if (fp) {
		fseek(fp, 0, 2);
		u64 fileSize = ftell(fp);
		fseek(fp, 0, 0);
        
		res.size = fileSize + 1;
		res.data = (u8*)calloc(fileSize + 1, 1);
        
		fread(res.data, 1, fileSize, fp);
        
		fclose(fp);
	}
    
	return(res);
}

void FreeDataBuffer(Data_Buffer* dataBuffer){
	if (dataBuffer->data) {
		free(dataBuffer->data);
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
    
    Data_Buffer buf = ReadFileToDataBuffer(filePath);
    
    std::vector<std::string> strings;
    
    char* at = (char*)buf.data;
    
    char bufStr[256];
    int inLinePos = 0;
    
    if(buf.size){
        
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
        
        result.count = strings.size();
        
        size_t totalDataNeeded = 0;
        for(int i = 0; i < strings.size(); i++){
            totalDataNeeded += strings[i].length() + 1 + sizeof(char*);
        }
        
        result.strings = (char**)malloc(totalDataNeeded);
        
        int curSumOfSizes = 0;
        for(int i = 0; i < result.count; i++){
            result.strings[i] = (char*)result.strings + result.count * sizeof(char*) + curSumOfSizes;
            curSumOfSizes += strings[i].length() + 1;
            strcpy(result.strings[i], strings[i].c_str());
        }
        
        FreeDataBuffer(&buf);
    }
    
    return(result);
}

void FreeStringList(Loaded_Strings* list){
    if(list->strings){
        free(list->strings);
        list->strings = 0;
    }
}

Bmp_Info AssetAllocateBitmapInternal(u32 width, u32 height, void* pixelsData) {
	Bmp_Info res = {};
    
	res.Width = width;
	res.Height = height;
	res.Pitch = 4 * width;
    
	res.WidthOverHeight = (float)width / (float)height;
    
	res.Pixels = (u8*)pixelsData;
    
	return(res);
}

Bmp_Info AssetAllocateBitmap(u32 width, u32 height) {
	u32 BitmapDataSize = width * height * 4;
	void* PixelsData = calloc(BitmapDataSize, 1);
    
	memset(PixelsData, 0, BitmapDataSize);
    
	Bmp_Info res = AssetAllocateBitmapInternal(width, height, PixelsData);
    
	return(res);
}

void AssetCopyBitmapData(Bmp_Info* Dst, Bmp_Info* Src) {
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

void AssetDeallocateBitmap(Bmp_Info* Buffer) {
	if (Buffer->Pixels) {
		free(Buffer->Pixels);
	}
}


Bmp_Info LoadBMP(char* FilePath){
    Bmp_Info res = {};
    
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
    res = AssetAllocateBitmapInternal(width, height, OurImageMem);
    
    int pixelsCountForSIMD = pixelsCount & (~3);
    
    __m128 mm255 = _mm_set1_ps(255.0f);
    __m128 mmOneOver255 = _mm_set1_ps(1.0f / 255.0f);
    __m128i mmFF = _mm_set1_epi32(0xFF);
    
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
    
    stbi_image_free(Image);
    
    return(res);
}


void LoadFontAddCodepoint(
Font_Info* FontInfo, 
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
    Glyph_Info* glyph = &FontInfo->Glyphs[glyphIndex];
    
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
	glyph->Bitmap = AssetAllocateBitmap(glyph->Width, glyph->Height);
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
    
    Bmp_Info CharBitmap= AssetAllocateBitmap(CharWidth, CharHeight);
	
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
		Bmp_Info ToBlur = AssetAllocateBitmap(
			2 * CharBorder + CharWidth,
			2 * CharBorder + CharHeight);
        
		Bmp_Info BlurredResult = AssetAllocateBitmap(
			2 * CharBorder + CharWidth,
			2 * CharBorder + CharHeight);
        
		Bmp_Info TempBitmap = AssetAllocateBitmap(
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
				if (resColor.a > 0.05f) {
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
        
		AssetDeallocateBitmap(&TempBitmap);
		AssetDeallocateBitmap(&ToBlur);
		AssetDeallocateBitmap(&BlurredResult);
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
    
    AssetDeallocateBitmap(&CharBitmap);
    stbtt_FreeBitmap(Bitmap, 0);
}

Font_Info LoadFont(char* FilePath, float height, u32 Flags){
    Font_Info res = {};
    
    stbtt_fontinfo STBFont;
    
    Data_Buffer TTFBuffer = ReadFileToDataBuffer(FilePath);
    stbtt_InitFont(&STBFont, TTFBuffer.data, 
                   stbtt_GetFontOffsetForIndex(TTFBuffer.data, 0));
    
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
    
    FreeDataBuffer(&TTFBuffer);
    
    return(res);
}

Mesh_Info MakeMesh(
std::vector<v3>& Positions,
std::vector<v2>& TexCoords,
std::vector<v3>& Normals,
std::vector<v3>& Tangents,
std::vector<v3>& Colors,
std::vector<u32> Indices,
b32 CalculateNormals,
b32 CalculateTangents)
{
	Mesh_Info Result = {};
    
    u32 VerticesCount = Positions.size();
    
	Result.Handle = 0;
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
    size_t VertsSize = sizeof(Vertex_Info) * VerticesCount;
	Result.Vertices = malloc(VertsSize);
    memset(Result.Vertices, 0, VertsSize);
    
    ASSERT(Positions.size());
    ASSERT(TexCoords.size());
    
    Vertex_Info* DstVerts = (Vertex_Info*)Result.Vertices;
    
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
    
    // NOTE(Dima): Writing vertices
    for(int VertexIndex = 0;
        VertexIndex < VerticesCount;
        VertexIndex++)
    {
        Vertex_Info* Vertex = (Vertex_Info*)Result.Vertices;
        
        *Vertex = {};
        
        Vertex->P = Positions[VertexIndex];
        
        if(TexCoords.size() == VerticesCount){
            Vertex->UV = TexCoords[VertexIndex];
        }
        
        if(Normals.size() == VerticesCount){
            Vertex->N = Normals[VertexIndex];
        }
    }
    
    return(Result);
}

Mesh_Info MakeSphere(int Segments, int Rings) {
    Mesh_Info Result = {};
    
    float Radius = 0.5f;
    
    Segments = Max(Segments, 3);
    Rings = Max(Rings, 2);
    
    //NOTE(dima): 2 top and bottom triangle fans + 
    int VerticesCount = (Segments * 3) * 2 + (Segments * (Rings - 2)) * 4;
    int IndicesCount = (Segments * 3) * 2 + (Segments * (Rings - 2)) * 6;
    
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
                      std::vector<v3>(),
                      Indices,
                      JOY_FALSE, 
                      JOY_TRUE);
    
    return(Result);
}

Mesh_Info GenerateCylynder(float Height, float Radius, int SidesCount) {
    Mesh_Info Result = {};
    
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
                      std::vector<v3>(),
                      Indices,
                      JOY_FALSE, 
                      JOY_TRUE);
    
    
    return(Result);
}