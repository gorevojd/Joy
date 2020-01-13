#ifndef JOY_ASSET_TYPES_H
#define JOY_ASSET_TYPES_H

#include "joy_types.h"
#include "joy_math.h"

struct Bmp_Info{
    void* Pixels;
    int Width;
    int Height;
    int Pitch;
    float WidthOverHeight;
    
    mi Handle;
};

struct Glyph_Info{
    int Codepoint;
    
    int Width;
    int Height;
    
    Bmp_Info Bitmap;
    
    /*Theese are offset from glyph origin to top-left of bitmap*/
	float XOffset;
	float YOffset;
	float Advance;
	float LeftBearingX;
    
    v2 MinUV;
    v2 MaxUV;
};


#if 0
enum Vertex_Layout_Type {
    VertexLayout_P,
	VertexLayout_PUV,
	VertexLayout_PUVN,
	VertexLayout_PNUV,
    VertexLayout_PUVNT,
    VertexLayout_PUVNTC,
	VertexLayout_PUVNC,
	VertexLayout_PNUVC,
};


inline size_t GetComponentCount4VertLayout(u32 VertLayout){
	u32 ComponentCount = 0;
	switch (VertLayout) {
        case VertexLayout_P:{
            ComponentCount = 3;
        }break;
        
		case VertexLayout_PUV: {
			ComponentCount = 5;
		}break;
        
		case VertexLayout_PNUV:
		case VertexLayout_PUVN: {
			ComponentCount = 8;
		}break;
        
        case VertexLayout_PUVNT:
		case VertexLayout_PUVNC:
		case VertexLayout_PNUVC: {
			ComponentCount = 11;
		}break;
        
        case VertexLayout_PUVNTC:{
            ComponentCount = 14;
        }break;
	}
    
    return(ComponentCount);
}
#endif


struct Vertex_Info{
    v3 P;
    v2 UV;
    v3 N;
    v3 T;
    v3 C;
};

struct Vertex_Skinned_Info{
    v3 P;
    v2 UV;
    v3 N;
    v3 T;
    v3 C;
};

enum Mesh_Type{
    Mesh_Simple,
    Mesh_Skinned,
};

struct Mesh_Info{
    size_t Handle;
    
    u32* Indices;
    int IndicesCount;
    
    void* Vertices;
    int VerticesCount;
    
    u32 MeshType;
};

#define FONT_INFO_MAX_GLYPH_COUNT 256
struct Font_Info{
    float AscenderHeight;
	float DescenderHeight;
	float LineGap;
    
    float* KerningPairs;
    
    Glyph_Info Glyphs[FONT_INFO_MAX_GLYPH_COUNT];
    int GlyphCount;
    
    int Codepoint2Glyph[FONT_INFO_MAX_GLYPH_COUNT];
    
    Bmp_Info AtlasImage;
};

#endif