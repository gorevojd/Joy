#ifndef TOOL_ASSET_BUILD_ASSIMP
#define TOOL_ASSET_BUILD_ASSIMP

#include "tool_asset_build_types.h"
#include "tool_asset_build_loading.h"
#include "tool_asset_build_commands.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <string>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

GLOBAL_VARIABLE aiTextureType SupportedTexturesTypes[] = {
    aiTextureType_DIFFUSE,
    aiTextureType_SPECULAR,
    aiTextureType_AMBIENT,
    aiTextureType_EMISSIVE,
    aiTextureType_HEIGHT,
    aiTextureType_NORMALS,
    aiTextureType_SHININESS,
    aiTextureType_OPACITY,
    aiTextureType_DISPLACEMENT,
    aiTextureType_LIGHTMAP,
    aiTextureType_REFLECTION,
    aiTextureType_UNKNOWN,
};

struct loaded_mat_texture{
    aiTextureType AiType;
    tool_bmp_info Bmp;
    u32 StoredBitmapID;
};

struct loaded_mat{
    int TextureFirstIndexOfTypeInArray[ArrayCount(SupportedTexturesTypes)];
    int TextureCountOfType[ArrayCount(SupportedTexturesTypes)];
    
    /*
I store here an array of paths to textures.
FirstIDInArray serves as mapping to get first ID texture
of specific texture type
*/
    std::vector<std::string> TexturePathArray;
};

enum assimp_load_mesh_flags {
	AssimpLoadMesh_GenerateNormals = 1,
	AssimpLoadMesh_GenerateTangents = 2,
	AssimpLoadMesh_GenerateSmoothNormals = 4,
};

struct loaded_model{
    std::vector<tool_mesh_info> Meshes;
    std::vector<loaded_mat> Materials;
};

struct load_model_source{
    char* Path;
    u32 AssetGroup;
    u32 Flags;
    
    loaded_model LoadedModel;
};

inline load_model_source ModelSource(
char* Path,
u32 AssetGroup,
u32 Flags)
{
    load_model_source Result = {};
    
    Result.Path = Path;
    Result.AssetGroup = AssetGroup;
    Result.Flags = Flags;
    
    return(Result);
}

struct model_loading_context{
    std::unordered_map<std::string, loaded_mat_texture> PathToTextureMap;
    
    std::vector<load_model_source> ModelSources;
};

inline m44 Assimp2JoyMatrix(const aiMatrix4x4& AssimpMatrix) {
	m44 Result = {};
    
	Result.e[0] = AssimpMatrix.a1;
	Result.e[1] = AssimpMatrix.a2;
	Result.e[2] = AssimpMatrix.a3;
	Result.e[3] = AssimpMatrix.a4;
	Result.e[4] = AssimpMatrix.b1;
	Result.e[5] = AssimpMatrix.b2;
	Result.e[6] = AssimpMatrix.b3;
	Result.e[7] = AssimpMatrix.b4;
	Result.e[8] = AssimpMatrix.c1;
	Result.e[9] = AssimpMatrix.c2;
	Result.e[10] = AssimpMatrix.c3;
	Result.e[11] = AssimpMatrix.c4;
	Result.e[12] = AssimpMatrix.d1;
	Result.e[13] = AssimpMatrix.d2;
	Result.e[14] = AssimpMatrix.d3;
	Result.e[15] = AssimpMatrix.d4;
    
	return(Result);
}

inline v3 Assimp2JoyVector3(const aiVector3D& AssimpVector) {
	v3 Result = {};
    
	Result.x = AssimpVector.x;
	Result.y = AssimpVector.y;
	Result.z = AssimpVector.z;
    
	return(Result);
}

inline v2 Assimp2JoyVector2(const aiVector2D AssimpVector) {
	v2 Result = {};
    
	Result.x = AssimpVector.x;
	Result.y = AssimpVector.y;
    
	return(Result);
}

inline v4 Assimp2JoyVector4(const aiColor4D& AssimpVector) {
	v4 Result = {};
    
	Result.x = AssimpVector.r;
	Result.y = AssimpVector.g;
	Result.z = AssimpVector.b;
	Result.w = AssimpVector.a;
    
	return(Result);
}

#endif