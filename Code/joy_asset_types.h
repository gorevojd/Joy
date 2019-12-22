#ifndef JOY_ASSET_TYPES_H
#define JOY_ASSET_TYPES_H

struct Bmp_Info{
    void* pixels;
    int width;
    int height;
    int pitch;
    float widthOverHeight;
};

struct Glyph_Info{
    int codepoint;
    
    int width;
    int height;
    
    Bmp_Info bitmap;
    
    /*Theese are offset from glyph origin to top-left of bitmap*/
	float xOffset;
	float yOffset;
	float advance;
	float leftBearingX;
};

#define FONT_INFO_MAX_GLYPH_COUNT 256
struct Font_Info{
    float ascenderHeight;
	float descenderHeight;
	float lineGap;
    
    float* kerningPairs;
    
    Glyph_Info glyphs[FONT_INFO_MAX_GLYPH_COUNT];
    int glyphCount;
    
    int codepoint2Glyph[FONT_INFO_MAX_GLYPH_COUNT];
    
    Bmp_Info atlasImage;
};

#endif