#ifndef JOY_ASSET_TYPES_H
#define JOY_ASSET_TYPES_H

struct bmp_info{
    void* Pixels;
    int Width;
    int Height;
    int Pitch;
    float WidthOverHeight;
};

struct glyph_info{
    int Codepoint;
    
    int Width;
    int Height;
    
    bmp_info Bitmap;
    
    /*Theese are offset from glyph origin to top-left of bitmap*/
	float XOffset;
	float YOffset;
	float Advance;
	float LeftBearingX;
};

#define FONT_INFO_MAX_GLYPH_COUNT 256
struct font_info{
    float AscenderHeight;
	float DescenderHeight;
	float LineGap;
    
    float* KerningPairs;
    
    glyph_info Glyphs[FONT_INFO_MAX_GLYPH_COUNT];
    int GlyphCount;
    
    int Codepoint2Glyph[FONT_INFO_MAX_GLYPH_COUNT];
    
    bmp_info AtlasImage;
};

#endif