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

data_buffer ReadFileToDataBuffer(char* FileName){
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

void FreeDataBuffer(data_buffer* DataBuffer){
	if (DataBuffer->Data) {
		free(DataBuffer->Data);
	}
}


bmp_info AssetAllocateBitmapInternal(u32 Width, u32 Height, void* PixelsData) {
	bmp_info Result = {};
    
	Result.Width = Width;
	Result.Height = Height;
	Result.Pitch = 4 * Width;
    
	Result.WidthOverHeight = (float)Width / (float)Height;
    
	Result.Pixels = (u8*)PixelsData;
    
	return(Result);
}

bmp_info AssetAllocateBitmap(u32 Width, u32 Height) {
	u32 BitmapDataSize = Width * Height * 4;
	void* PixelsData = calloc(BitmapDataSize, 1);
    
	memset(PixelsData, 0, BitmapDataSize);
    
	bmp_info Result = AssetAllocateBitmapInternal(Width, Height, PixelsData);
    
	return(Result);
}

void AssetCopyBitmapData(bmp_info* Dst, bmp_info* Src) {
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

void AssetDeallocateBitmap(bmp_info* Buffer) {
	if (Buffer->Pixels) {
		free(Buffer->Pixels);
	}
}


bmp_info LoadBMP(char* FilePath){
    bmp_info Result = {};
    
    int Width;
    int Height;
    int Channels;
    
    unsigned char* Image = stbi_load(
        FilePath,
        &Width,
        &Height,
        &Channels,
        STBI_rgb_alpha);
    
    int PixelsCount = Width * Height;
    int ImageSize = PixelsCount * 4;
    
    void* OurImageMem = malloc(ImageSize);
    Result = AssetAllocateBitmapInternal(Width, Height, OurImageMem);
    
    for(int PixelIndex = 0;
        PixelIndex < PixelsCount;
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
    
    stbi_image_free(Image);
    
    return(Result);
}


void LoadFontAddCodepoint(
font_info* FontInfo, 
stbtt_fontinfo* STBFont,
int Codepoint, 
int* AtlasWidth, 
int* AtlasHeight,
float FontScale,
u32 Flags,
int BlurRadius,
float* GaussianBox)
{
    int GlyphIndex = FontInfo->GlyphCount++;
    glyph_info* Glyph = &FontInfo->Glyphs[GlyphIndex];
    
    FontInfo->Codepoint2Glyph[Codepoint] = GlyphIndex;
    
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
    
    Glyph->Width = CharBmpWidth + ShadowOffset;
	Glyph->Height = CharBmpHeight + ShadowOffset;
	Glyph->Bitmap = AssetAllocateBitmap(Glyph->Width, Glyph->Height);
	Glyph->Advance = Advance * FontScale;
	Glyph->LeftBearingX = LeftBearingX * FontScale;
	Glyph->XOffset = XOffset - CharBorder;
	Glyph->YOffset = YOffset - CharBorder;
	Glyph->Codepoint = Codepoint;
    
    *AtlasWidth += Glyph->Width;
	*AtlasHeight = Max(*AtlasHeight, Glyph->Height);
    
    //NOTE(dima): Clearing the image bytes
	u32* Pixel = (u32*)Glyph->Bitmap.Pixels;
	for (int Y = 0; Y < Glyph->Height; Y++) {
		for (int X = 0; X < Glyph->Width; X++) {
			*Pixel++ = 0;
		}
	}
    
    //NOTE(dima): Forming char bitmap
	float OneOver255 = 1.0f / 255.0f;
    
    bmp_info CharBitmap= AssetAllocateBitmap(CharWidth, CharHeight);
	
	for (int j = 0; j < CharHeight; j++) {
		for (int i = 0; i < CharWidth; i++) {
			u8 Grayscale = *((u8*)Bitmap + j * CharWidth + i);
			float Grayscale01 = (float)Grayscale * OneOver255;
            
			v4 ResultColor = V4(1.0f, 1.0f, 1.0f, Grayscale01);
            
			/*Alpha premultiplication*/
			ResultColor.r *= ResultColor.a;
			ResultColor.g *= ResultColor.a;
			ResultColor.b *= ResultColor.a;
            
			u32 ColorValue = PackRGBA(ResultColor);
			u32* TargetPixel = (u32*)((u8*)CharBitmap.Pixels + j* CharBitmap.Pitch + i * 4);
			*TargetPixel = ColorValue;
		}
	}
    
    //NOTE(dima): Render blur if needed
	if (Flags & LoadFont_BakeBlur) {
		bmp_info ToBlur = AssetAllocateBitmap(
			2 * CharBorder + CharWidth,
			2 * CharBorder + CharHeight);
        
		bmp_info BlurredResult = AssetAllocateBitmap(
			2 * CharBorder + CharWidth,
			2 * CharBorder + CharHeight);
        
		bmp_info TempBitmap = AssetAllocateBitmap(
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
                
				v4 ResultColor = FromColor;
				if (ResultColor.a > 0.05f) {
                    ResultColor.a = 1.0f;
				}
                
				*ToPix = PackRGBA(ResultColor);
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
				v4 ResultColor = FromColor;
				if (ResultColor.a > 0.05f) {
					ResultColor.a = 1.0f;
				}
				*ToPix = PackRGBA(ResultColor);
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
			&Glyph->Bitmap,
			&BlurredResult,
			0, 0,
			V4(0.0f, 0.0f, 0.0f, 1.0f));
        
		AssetDeallocateBitmap(&TempBitmap);
		AssetDeallocateBitmap(&ToBlur);
		AssetDeallocateBitmap(&BlurredResult);
	}
    
    if(Flags & LoadFont_BakeShadow){
        
        RenderOneBitmapIntoAnother(
            &Glyph->Bitmap,
            &CharBitmap,
            CharBorder + ShadowOffset,
            CharBorder + ShadowOffset,
            V4(0.0f, 0.0f, 0.0f, 1.0f));
        
    }
    
    RenderOneBitmapIntoAnother(
        &Glyph->Bitmap,
        &CharBitmap,
        CharBorder,
        CharBorder,
        V4(1.0f, 1.0f, 1.0f, 1.0f));
    
    AssetDeallocateBitmap(&CharBitmap);
    stbtt_FreeBitmap(Bitmap, 0);
}

font_info LoadFont(char* FilePath, float Height, u32 Flags){
    font_info Result = {};
    
    stbtt_fontinfo STBFont;
    
    data_buffer TTFBuffer = ReadFileToDataBuffer(FilePath);
    stbtt_InitFont(&STBFont, TTFBuffer.Data, 
                   stbtt_GetFontOffsetForIndex(TTFBuffer.Data, 0));
    
    float Scale = stbtt_ScaleForPixelHeight(&STBFont, Height);
    
    int AscenderHeight;
	int DescenderHeight;
	int LineGap;
    
    stbtt_GetFontVMetrics(
		&STBFont,
		&AscenderHeight,
		&DescenderHeight,
		&LineGap);
    
    Result.AscenderHeight = (float)AscenderHeight * Scale;
	Result.DescenderHeight = (float)DescenderHeight * Scale;
	Result.LineGap = (float)LineGap * Scale;
    
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
            &Result, 
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
	Result.KerningPairs = (float*)malloc(sizeof(float) * Result.GlyphCount * Result.GlyphCount);
    
	for (int FirstGlyphIndex = 0; FirstGlyphIndex < Result.GlyphCount; FirstGlyphIndex++) {
		for (int SecondGlyphIndex = 0; SecondGlyphIndex < Result.GlyphCount; SecondGlyphIndex++) {
			u32 KerningIndex = SecondGlyphIndex * Result.GlyphCount + FirstGlyphIndex;
            
			int FirstCodepoint = Result.Glyphs[FirstGlyphIndex].Codepoint;
			int SecondCodepoint = Result.Glyphs[SecondGlyphIndex].Codepoint;
            
			int Kern = stbtt_GetGlyphKernAdvance(&STBFont, FirstCodepoint, SecondCodepoint);
            
			Result.KerningPairs[KerningIndex] = (float)Kern * Scale;
		}
	}
    
    FreeDataBuffer(&TTFBuffer);
    
    return(Result);
}

