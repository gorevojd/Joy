#ifndef JOY_RENDER_PRIMITIVES_H
#define JOY_RENDER_PRIMITIVES_H

enum bitmap_data_layout
{
    BmpDataLayout_RGBA,
    BmpDataLayout_RG,
    BmpDataLayout_Grayscale,
};

struct render_primitive_bitmap
{
    void* Data;
    u32 LayoutType;
    
    int Width;
    int Height;
    
    // NOTE(Dima): In atlas info
    v2 MinUV;
    v2 MaxUV;
    
    int Pitch;
    float WidthOverHeight;
    
    // NOTE(Dima): Handle reserved for Graphics API
    mi Handle;
};

enum material_channel_type
{
    MaterialChannel_Albedo,
    MaterialChannel_Ambient,
    MaterialChannel_Specular,
    MaterialChannel_Normal,
    MaterialChannel_Metal,
    MaterialChannel_Roughness,
    MaterialChannel_Emissive,
    
    MaterialChannel_Count,
};

struct render_primitive_material
{
    v3 ModColor;
    
    v3 Colors[MaterialChannel_Count];
    render_primitive_bitmap* Textures[MaterialChannel_Count];
};

enum mesh_handles_types{
    MeshHandle_None,
    MeshHandle_VertexArray,
    MeshHandle_Buffer,
};

#define MESH_HANDLES_COUNT 8
struct mesh_handles{
    mi Handles[MESH_HANDLES_COUNT];
    u32 HandlesTypes[MESH_HANDLES_COUNT];
    int Count;
    
    b32 Allocated;
};

enum mesh_type{
    Mesh_Simple,
    Mesh_Skinned,
};

struct mesh_type_context{
    u16 VertexTypeSize;
    u16 OffsetP;
    u16 OffsetUV;
    u16 OffsetN;
    u16 OffsetT;
    u16 OffsetWeights;
    u16 OffsetBoneIDs;
    
    u8 MeshType;
};

struct render_primitive_mesh
{
    u32* Indices;
    int IndicesCount;
    
    void* Vertices;
    int VerticesCount;
    
    mesh_type_context TypeCtx;
    mesh_handles Handles;
};

#endif //JOY_RENDER_PRIMITIVES_H
