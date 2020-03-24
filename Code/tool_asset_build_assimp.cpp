#include "tool_asset_build_assimp.h"

/*
This function should return the first index of Type textures that
were pushed in array
*/
INTERNAL_FUNCTION int AiLoadMatTexturesForType(loaded_model* Model,
                                               loaded_mat* OurMat,
                                               loading_context* Ctx,
                                               aiMaterial* Mat,
                                               aiTextureType Type)
{
    int Result = -1;
    
    for(int TextureIndex = 0;
        TextureIndex < Mat->GetTextureCount(Type);
        TextureIndex++)
    {
        aiString TexturePath;
        Mat->GetTexture(Type, TextureIndex, &TexturePath);
        // TODO(Dima): Make sure that this is correct
        std::string TexturePathNew(TexturePath.C_Str());
        
        auto* Mapping = &Ctx->PathToTextureMap;
        
        b32 NotLoaded = (Mapping->find(TexturePathNew) == Mapping->end());
        if(NotLoaded){
            loaded_mat_texture Tex;
            Tex.Bmp = LoadBMP((char*)TexturePathNew.c_str());
            Tex.AiType = Type;
            
            Mapping->insert(std::make_pair(TexturePathNew, Tex));
        }
        
        // NOTE(Dima): If this is the first texture of type 
        // NOTE(Dima): than return it
        if(TextureIndex == 0){
            Result = OurMat->BmpArray.size();
        }
        OurMat->BmpArray.push_back(TexturePathNew);
    }
    
    return(Result);
}

loaded_model LoadModelByASSIMP(char* FileName, u32 Flags, loading_context* LoadingCtx)
{
    loaded_model Result = {};
    
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
    
    //NOTE(Dima): Loading materials
    for(int MatIndex = 0; MatIndex < scene->mNumMaterials;MatIndex++){
        aiMaterial* AssimpMaterial = scene->mMaterials[MatIndex];
        
        loaded_mat NewMaterial = {};
        
        // NOTE(Dima): Loading textures for supported Assimp textures types
        for(int TTypeIndex = 0;
            TTypeIndex < ARRAY_COUNT(SupportedTexturesTypes);
            TTypeIndex++)
        {
            NewMaterial.FirstIDInArray[TTypeIndex] = AiLoadMatTexturesForType(
                &Result, 
                &NewMaterial,
                LoadingCtx, 
                AssimpMaterial, 
                SupportedTexturesTypes[TTypeIndex]);
        }
        
        Result.Materials.push_back(NewMaterial);
    }
    
    // NOTE(Dima): Loading meshes
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
        
		tool_mesh_info Mesh = MakeMesh(
			Vertices,
			TexCoords,
			Normals,
			Tangents,
			Colors,
			Indices,
			JoyShouldCalculateNormals,
			JoyShouldCalculateTangents);
        
        Result.Meshes.push_back(Mesh);
    }
    
    return(Result);
}

INTERNAL_FUNCTION void StoreLoadingContext(asset_system* System, 
                                           loading_context* LoadingCtx)
{
    BeginAsset(System, GameAsset_Type_Bitmap);
    
    u32 FirstBmpID = 0;
    int Index = 0;
    for(auto it: LoadingCtx->PathToTextureMap){
        added_asset Added = AddBitmapAssetManual(System, &it.second.Bmp);
        
        if(Index = 0){
            FirstBmpID = Added.ID;
        }
    }
    EndAsset(System);
}

INTERNAL_FUNCTION void StoreModelAsset(asset_system* System,
                                       u32 AssetGroupID,
                                       loaded_model* Model)
{
    BeginAsset(System, GameAsset_Type_Mesh);
    for(auto it: Model->Meshes){
        AddMeshAsset(System, &it);
    }
    EndAsset(System);
    
    BeginAsset(System, AssetGroupID);
    
    EndAsset(System);
}

INTERNAL_FUNCTION void WriteMeshes1(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    loading_context LoadCtx = {};
    
    u32 DefaultFlags = 
        AssimpLoadMesh_GenerateNormals |
        AssimpLoadMesh_GenerateTangents;
    // NOTE(Dima): Here load all models
    
    loaded_model BathroomModel = LoadModelByASSIMP(
        "../Data/Models/Bathroom.fbx", 
        DefaultFlags,
        &LoadCtx);
    
    loaded_model HeartModel = LoadModelByASSIMP(
        "../Data/Models/Heart.fbx",
        DefaultFlags,
        &LoadCtx);
    
    loaded_model KindPlaneModel = LoadModelByASSIMP(
        "../Data/Models/KindPlane.fbx",
        DefaultFlags, 
        &LoadCtx);
    
    loaded_model PodkovaModel = LoadModelByASSIMP(
        "../Data/Models/Podkova.fbx", DefaultFlags, 
        &LoadCtx);
    
    loaded_model RBinBigModel = LoadModelByASSIMP(
        "../Data/Models/RubbishBin.fbx", 
        DefaultFlags, 
        &LoadCtx);
    
    loaded_model SnowmanModel = LoadModelByASSIMP(
        "../Data/Models/snowman.fbx", 
        DefaultFlags, 
        &LoadCtx);
    
    loaded_model StoolModel = LoadModelByASSIMP(
        "../Data/Models/stool.fbx", 
        DefaultFlags, 
        &LoadCtx);
    
    loaded_model ToiletModel = LoadModelByASSIMP(
        "../Data/Models/toilet.fbx", 
        DefaultFlags, 
        &LoadCtx);
    
    loaded_model VaseModel1 = LoadModelByASSIMP(
        "../Data/Models/vaza1.fbx",
        DefaultFlags,
        &LoadCtx);
    
    // NOTE(Dima): Storing loading context
    StoreLoadingContext(System, &LoadCtx);
    
    // NOTE(Dima): Here store all models in asset file
    StoreModelAsset(System, GameAsset_Stool, &StoolModel);
    StoreModelAsset(System, GameAsset_Bathroom, &BathroomModel);
    StoreModelAsset(System, GameAsset_Heart, &HeartModel);
    StoreModelAsset(System, GameAsset_KindPlane, &KindPlaneModel);
    StoreModelAsset(System, GameAsset_Podkova, &PodkovaModel);
    StoreModelAsset(System, GameAsset_RubbishBin, &RBinBigModel);
    StoreModelAsset(System, GameAsset_Snowman, &SnowmanModel);
    StoreModelAsset(System, GameAsset_Toilet, &ToiletModel);
    StoreModelAsset(System, GameAsset_Vase, &VaseModel1);
    
    WriteAssetFile(System, "../Data/AssimpMeshes1.ja");
}

int main(int ArgsCount, char** Args){
    
    
    
    system("pause");
    return(0);
}