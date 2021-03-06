#ifndef JOY_ASSET_TYPES_H
#define JOY_ASSET_TYPES_H

#include "joy_asset_types_shared.h"

struct array_info{
    u32 FirstID;
    int Count;
};

struct bmp_info{
    render_primitive_bitmap Prim;
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

struct mesh_info{
    render_primitive_mesh Prim;
    
    int MaterialIndexInModel;
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

struct node_info{
    node_shared_data* Shared;
    
    u32* MeshIDs;
    int MeshCount;
    
    m44 CalculatedToParent;
    m44 CalculatedToModel;
};

struct skeleton_info{
    bone_info* Bones;
    int BoneCount;
};

struct material_info{
    //u32 BitmapIDs[MaterialTexture_Count];
    
    //v3 Colors[MaterialTexture_Count];
    //bmp_info* Textures[MaterialTexture_Count];
    
    render_primitive_material Prim;
};

struct model_info{
    u32* MeshIDs;
    u32* MaterialIDs;
    u32* AnimationIDs;
    node_info* Nodes;
    u32* NodeMeshIDsStorage;
    node_shared_data* NodesSharedDatas;
    
    u32 NodesCheckSum;
    
    b32 HasSkeleton;
    u32 SkeletonID;
    
    int MeshCount;
    int MaterialCount;
    int NodeCount;
    int AnimationCount;
    int NodesMeshIDsStorageCount;
};

struct node_animation{
    // NOTE(Dima): These are precalculated values for start&end in animation
    v3 BeginP;
    v3 EndP;
    
    quat BeginR;
    quat EndR;
    
    v3 BeginS;
    v3 EndS;
    
    // NOTE(Dima): Data
    float* PositionKeysTimes;
    v3* PositionKeysValues;
    
    float* RotationKeysTimes;
    quat* RotationKeysValues;
    
    float* ScalingKeysTimes;
    v3* ScalingKeysValues;
    
    int PositionKeysCount;
    int RotationKeysCount;
    int ScalingKeysCount;
    b32 IsRootMotion;
    
    // NOTE(Dima): this is reference index to NodesSharedDatas
    int NodeIndex;
};

struct animation_clip{
    float DurationTicks;
    float TicksPerSecond;
    
    char* Name;
    
    u32 NodesCheckSum;
    
    u32* NodeAnimationIDs;
    int NodeAnimationsCount;
    
    b32 UsesRootMotion;
    u32 RootMotionNodeAnimID;
};

#endif