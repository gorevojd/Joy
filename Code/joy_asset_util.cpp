#include "joy_asset_util.h"
#include "joy_software_renderer.h"
#include "joy_defines.h"
#include "joy_render_blur.h"

#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_STATIC
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

Data_Buffer ReadFileToDataBuffer(char* fileName){
	Data_Buffer res = {};
    
	FILE* fp = fopen(fileName, "rb");
	if (fp) {
		fseek(fp, 0, 2);
		u64 fileSize = ftell(fp);
		fseek(fp, 0, 0);
        
		res.size = fileSize;
		res.data = (u8*)calloc(fileSize, 1);
        
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


Bmp_Info AssetAllocateBitmapInternal(u32 width, u32 height, void* pixelsData) {
	Bmp_Info res = {};
    
	res.width = width;
	res.height = height;
	res.pitch = 4 * width;
    
	res.widthOverHeight = (float)width / (float)height;
    
	res.pixels = (u8*)pixelsData;
    
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
	Assert(Dst->width == Src->width);
	Assert(Dst->height == Src->height);
    
	u32* DestOut = (u32*)Dst->pixels;
	u32* ScrPix = (u32*)Src->pixels;
	for (int j = 0; j < Src->height; j++) {
		for (int i = 0; i < Src->width; i++) {
			*DestOut++ = *ScrPix++;
		}
	}
}

void AssetDeallocateBitmap(Bmp_Info* Buffer) {
	if (Buffer->pixels) {
		free(Buffer->pixels);
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
        
        *((u32*)res.pixels + PixelIndex) = PackedColor;
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
    int glyphIndex = FontInfo->glyphCount++;
    Glyph_Info* glyph = &FontInfo->glyphs[glyphIndex];
    
    FontInfo->codepoint2Glyph[Codepoint] = glyphIndex;
    
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
    
    glyph->width = CharBmpWidth + ShadowOffset;
	glyph->height = CharBmpHeight + ShadowOffset;
	glyph->bitmap = AssetAllocateBitmap(glyph->width, glyph->height);
	glyph->advance = Advance * FontScale;
	glyph->leftBearingX = LeftBearingX * FontScale;
	glyph->xOffset = XOffset - CharBorder;
	glyph->yOffset = YOffset - CharBorder;
	glyph->codepoint = Codepoint;
    
    *AtlasWidth += glyph->width;
	*AtlasHeight = Max(*AtlasHeight, glyph->height);
    
    //NOTE(dima): Clearing the image bytes
	u32* Pixel = (u32*)glyph->bitmap.pixels;
	for (int Y = 0; Y < glyph->height; Y++) {
		for (int X = 0; X < glyph->width; X++) {
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
			u32* TargetPixel = (u32*)((u8*)CharBitmap.pixels + j* CharBitmap.pitch + i * 4);
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
			BlurredResult.pixels,
			ToBlur.width,
			ToBlur.height,
			BlurRadius,
			GaussianBox);
        
        
		for (int Y = 0; Y < ToBlur.height; Y++) {
			for (int X = 0; X < ToBlur.width; X++) {
				u32* FromPix = (u32*)BlurredResult.pixels + Y * BlurredResult.width + X;
				u32* ToPix = (u32*)ToBlur.pixels + Y * ToBlur.width + X;
                
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
			BlurredResult.pixels,
			ToBlur.width,
			ToBlur.height,
			BlurRadius,
			GaussianBox);
        
#else
		BlurBitmapApproximateGaussian(
			&ToBlur,
			BlurredResult.pixels,
			TempBitmap.pixels,
			ToBlur.width,
			ToBlur.height,
			BlurRadius);
		for (int Y = 0; Y < ToBlur.height; Y++) {
			for (int X = 0; X < ToBlur.width; X++) {
				u32* FromPix = (u32*)BlurredResult.pixels + Y * BlurredResult.width + X;
				u32* ToPix = (u32*)ToBlur.pixels + Y * ToBlur.width + X;
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
			BlurredResult.pixels,
			TempBitmap.pixels,
			ToBlur.width,
			ToBlur.height,
			BlurRadius);
#endif
        
		RenderOneBitmapIntoAnother(
			&glyph->bitmap,
			&BlurredResult,
			0, 0,
			V4(0.0f, 0.0f, 0.0f, 1.0f));
        
		AssetDeallocateBitmap(&TempBitmap);
		AssetDeallocateBitmap(&ToBlur);
		AssetDeallocateBitmap(&BlurredResult);
	}
    
    if(Flags & LoadFont_BakeShadow){
        
        RenderOneBitmapIntoAnother(
            &glyph->bitmap,
            &CharBitmap,
            CharBorder + ShadowOffset,
            CharBorder + ShadowOffset,
            V4(0.0f, 0.0f, 0.0f, 1.0f));
        
    }
    
    RenderOneBitmapIntoAnother(
        &glyph->bitmap,
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
    
    res.ascenderHeight = (float)AscenderHeight * Scale;
	res.descenderHeight = (float)DescenderHeight * Scale;
	res.lineGap = (float)LineGap * Scale;
    
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
	res.kerningPairs = (float*)malloc(sizeof(float) * res.glyphCount * res.glyphCount);
    
	for (int firstIndex = 0; firstIndex < res.glyphCount; firstIndex++) {
		for (int secondIndex = 0; secondIndex < res.glyphCount; secondIndex++) {
			u32 KerningIndex = secondIndex * res.glyphCount + firstIndex;
            
			int FirstCodepoint = res.glyphs[firstIndex].codepoint;
			int SecondCodepoint = res.glyphs[secondIndex].codepoint;
            
			int Kern = stbtt_GetGlyphKernAdvance(&STBFont, FirstCodepoint, SecondCodepoint);
            
			res.kerningPairs[KerningIndex] = (float)Kern * Scale;
		}
	}
    
    FreeDataBuffer(&TTFBuffer);
    
    return(res);
}

