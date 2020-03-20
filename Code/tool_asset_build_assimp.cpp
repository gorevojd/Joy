#include "tool_asset_build_assimp.h"

void LoadModelByASSIMP(char* FileName, u32 Flags) {
    
	Assimp::Importer importer;
    
	b32 JoyShouldCalculateNormals = 0;
	b32 JoyShouldCalculateTangents = 0;
    
	u64 AssimpFlags = 
		aiProcess_OptimizeMeshes |
		aiProcess_OptimizeGraph |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType;
    
	if (Flags & AssimpLoadMesh_GenerateTangents) {
		AssimpFlags |= aiProcess_CalcTangentSpace;
	}
	else {
		JoyShouldCalculateTangents = true;
	}
    
	if (Flags & AssimpLoadMesh_GenerateNormals) {
		AssimpFlags |= aiProcess_GenNormals;
	}
	else {
		JoyShouldCalculateNormals = true;
	}
    
	if (Flags & AssimpLoadMesh_GenerateSmoothNormals) {
		AssimpFlags |= aiProcess_GenSmoothNormals;
	}
	else {
		JoyShouldCalculateNormals = true;
	}
    
	const aiScene* scene = importer.ReadFile(
		FileName,
		AssimpFlags);
    
	if (scene) {
		printf("Model %s has been successfully loaded by ASSIMP\n", FileName);
	}
	else {
		printf("Error while reading %s by ASSIMP\n", FileName);
	}
    
	int NumMeshes = scene->mNumMeshes;
	for (int MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++) {
		aiMesh* AssimpMesh = scene->mMeshes[MeshIndex];
        
		std::vector<v3> Vertices;
		std::vector<v3> Normals;
		std::vector<v2> TexCoords;
		std::vector<v3> Tangents;
		std::vector<v3> Colors;
		std::vector<u32> Indices;
        
		int NumColorChannels = AssimpMesh->GetNumColorChannels();
		int NumUVChannels = AssimpMesh->GetNumUVChannels();
        
		//NOTE(dima): Getting indices
		for (int FaceIndex = 0; FaceIndex < AssimpMesh->mNumFaces; FaceIndex++) {
			aiFace* AssimpFace = &AssimpMesh->mFaces[FaceIndex];
            
			int NumberOfIndicesInPrimitive = AssimpFace->mNumIndices;
			if (NumberOfIndicesInPrimitive == 3) {
				Indices.push_back(AssimpFace->mIndices[0]);
				Indices.push_back(AssimpFace->mIndices[1]);
				Indices.push_back(AssimpFace->mIndices[2]);
			}
		}
        
		//NOTE(Dima): Getting colors
		if (NumColorChannels) {
			int ColorChannelIndex = 0;
            
			for (int VIndex = 0; VIndex < AssimpMesh->mNumVertices; VIndex++) {
				v4 Color = Assimp2JoyVector4(AssimpMesh->mColors[ColorChannelIndex][VIndex]);
                
				Colors.push_back(Color.rgb);
			}
		}
        
		//NOTE(Dima): Getting UVs
		if (NumUVChannels) {
			int UVChannelIndex = 0;
            
			for (int VIndex = 0; VIndex < AssimpMesh->mNumVertices; VIndex++) {
				v3 TexCoord = Assimp2JoyVector3(AssimpMesh->mTextureCoords[UVChannelIndex][VIndex]);
                
				TexCoords.push_back(V2(TexCoord.x, TexCoord.y));
			}
		}
        
		//NOTE(dima): Getting normals, positions, tangents
		for (int VIndex = 0; VIndex < AssimpMesh->mNumVertices; VIndex++) {
			v3 P = Assimp2JoyVector3(AssimpMesh->mVertices[VIndex]);
			v3 T = Assimp2JoyVector3(AssimpMesh->mTangents[VIndex]);
			v3 N = Assimp2JoyVector3(AssimpMesh->mNormals[VIndex]);
            
			Vertices.push_back(P);
			Tangents.push_back(T);
			Normals.push_back(N);
		}
        
		//TODO: Further processing of the meshes
		tool_mesh_info Mesh = MakeMesh(
			Vertices,
			TexCoords,
			Normals,
			Tangents,
			Colors,
			Indices,
			JoyShouldCalculateNormals,
			JoyShouldCalculateTangents);
	}
}

int main(int ArgsCount, char** Args){
    
    
    
    system("pause");
    return(0);
}