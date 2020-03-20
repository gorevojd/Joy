#ifndef TOOL_ASSET_BUILD_ASSIMP
#define TOOL_ASSET_BUILD_ASSIMP

#include "tool_asset_build_types.h"
#include "tool_asset_build_loading.h"
#include "tool_asset_build_commands.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

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

enum assimp_load_mesh_flags {
	AssimpLoadMesh_GenerateNormals = 1,
	AssimpLoadMesh_GenerateTangents = 2,
	AssimpLoadMesh_GenerateSmoothNormals = 4,
};

#endif