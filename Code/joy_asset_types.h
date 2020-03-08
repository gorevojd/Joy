#ifndef JOY_ASSET_TYPES_H
#define JOY_ASSET_TYPES_H

#include "joy_types.h"
#include "joy_math.h"

struct bmp_array_info{
    u32 FirstBmpID;
    int Count;
};

struct bmp_info{
    void* Pixels;
    int Width;
    int Height;
    int Pitch;
    float WidthOverHeight;
    
    // NOTE(Dima): In atlas info
    v2 MinUV;
    v2 MaxUV;
    
    // NOTE(Dima): Handle reserved for Graphics API
    mi Handle;
};

struct sound_info{
    // NOTE(Dima): Left and Right channels
    i16* Samples[2];
    
    int Channels;
    int SampleCount;
    int SamplesPerSec;
};

struct Asset_Atlas{
    bmp_info Bitmap;
    int AtX;
    int AtY;
    int Dim;
    int MaxInRowHeight;
    
    float OneOverDim;
};

struct In_Atlas_Bitmap{
    v2 MinUV;
    v2 MaxUV;
    
    int Width;
    int Height;
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


struct vertex_info{
    v3 P;
    v2 UV;
    v3 N;
    v3 T;
    v3 C;
};

struct vertex_skinned_info{
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

enum mesh_handles_types{
    MeshHandle_None,
    MeshHandle_VertexArray,
    MeshHandle_Buffer,
};

#define MESH_HANDLES_COUNT 8
struct mesh_handles{
    size_t Handles[MESH_HANDLES_COUNT];
    int HandlesTypes[MESH_HANDLES_COUNT];
    int Count;
    
    b32 Allocated;
};

struct mesh_info{
    u32* Indices;
    int IndicesCount;
    
    void* Vertices;
    int VerticesCount;
    
    u32 MeshType;
    
    mesh_handles Handles;
};

struct glyph_info{
    int Codepoint;
    
    int Width;
    int Height;
    float WidthOverHeight;
    
    u32 BitmapID;
    
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
    
    u32* GlyphIDs;
    int GlyphCount;
    
    u8 Codepoint2Glyph[FONT_INFO_MAX_GLYPH_COUNT];
};

struct material_info{
    
};

struct model_info{
    
};

#endif