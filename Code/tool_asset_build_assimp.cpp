#include "tool_asset_build_assimp.h"

inline int ConvertAssimpToOurTextureType(u32 Assimp){
    int Result = MaterialTexture_Unknown;
    
    switch(Assimp){
        case aiTextureType_DIFFUSE:{
            Result = MaterialTexture_Diffuse;
        }break;
        
        case aiTextureType_SPECULAR:{
            Result = MaterialTexture_Specular;
        }break;
        
        case aiTextureType_AMBIENT:{
            Result = MaterialTexture_Ambient;
        }break;
        
        case aiTextureType_EMISSIVE:{
            Result = MaterialTexture_Emissive;
        }break;
        
        case aiTextureType_HEIGHT:{
            Result = MaterialTexture_Height;
        }break;
        
        case aiTextureType_NORMALS:{
            Result = MaterialTexture_Normals;
        }break;
        
        case aiTextureType_SHININESS:{
            Result = MaterialTexture_Shininess;
        }break;
        
        case aiTextureType_OPACITY:{
            Result = MaterialTexture_Opacity;
        }break;
        
        case aiTextureType_DISPLACEMENT:{
            Result = MaterialTexture_Displacement;
        }break;
        
        case aiTextureType_LIGHTMAP:{
            Result = MaterialTexture_Lightmap;
        }break;
        
        case aiTextureType_REFLECTION:{
            Result = MaterialTexture_Reflection;
        }break;
        
        case aiTextureType_UNKNOWN:{
            Result = MaterialTexture_Unknown;
        }break;
    }
    
    return(Result);
}

/*
This function should return the first index of Type textures that
were pushed in array
*/
struct assimp_loaded_textures_for_type{
    u32 FirstID;
    int Count;
};

INTERNAL_FUNCTION assimp_loaded_textures_for_type
AiLoadMatTexturesForType(loaded_model* Model,
                         loaded_mat* OurMat,
                         model_loading_context* Ctx,
                         aiMaterial* Mat,
                         aiTextureType Type)
{
    assimp_loaded_textures_for_type Result = {};
    
    int TextureCountForType = Mat->GetTextureCount(Type);
    
    Result.FirstID = -1;
    Result.Count = TextureCountForType;
    
    for(int TextureIndex = 0;
        TextureIndex < TextureCountForType;
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
            Result.FirstID = OurMat->TexturePathArray.size();
        }
        OurMat->TexturePathArray.push_back(TexturePathNew);
    }
    
    return(Result);
}

loaded_model LoadModelByASSIMP(char* FileName, u32 Flags, model_loading_context* LoadingCtx)
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
    
    b32 InGenNorm = Flags & AssimpLoadMesh_GenerateNormals;
    b32 InGenSmNorm = Flags & AssimpLoadMesh_GenerateSmoothNormals;
    
    if (InGenNorm || InGenSmNorm) 
    {
        if(InGenNorm){
            AssimpFlags |= aiProcess_GenNormals;
        }
        else{
            AssimpFlags |= aiProcess_GenSmoothNormals;
        }
        
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
            assimp_loaded_textures_for_type LoadedRes = AiLoadMatTexturesForType(
                &Result, 
                &NewMaterial,
                LoadingCtx, 
                AssimpMaterial, 
                SupportedTexturesTypes[TTypeIndex]);
            
            NewMaterial.TextureFirstIndexOfTypeInArray[TTypeIndex] = LoadedRes.FirstID;
            NewMaterial.TextureCountOfType[TTypeIndex] = LoadedRes.Count;
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
#if 0
                v4 Color = Assimp2JoyVector4(AssimpMesh->mColors[ColorChannelIndex][VIndex]);
#else
                v4 Color = V4(1.0f, 1.0f, 1.0f, 1.0f);
#endif
                
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

INTERNAL_FUNCTION tool_model_info LoadedToToolModelInfo(loaded_model* Model){
    tool_model_info Result = {};
    
    Result.MeshCount = Model->Meshes.size();
    Result.MaterialCount = Model->Materials.size();
    
    Result.MeshIDs = (u32*)malloc(sizeof(u32) * Model->Meshes.size());
    Result.MaterialIDs = (u32*)malloc(sizeof(u32) * Model->Meshes.size());
    
    return(Result);
}

INTERNAL_FUNCTION tool_material_info LoadedToToolMaterialInfo(loaded_mat* Material){
    tool_material_info Result = {};
    
    
    
    return(Result);
}

INTERNAL_FUNCTION void StoreModelAsset(asset_system* System,
                                       u32 AssetGroupID,
                                       loaded_model* Model,
                                       model_loading_context* Context)
{
    Model->ToolModelInfo = LoadedToToolModelInfo(Model);
    tool_model_info* ToolModel = &Model->ToolModelInfo;
    
    BeginAsset(System, GameAsset_Type_Mesh);
    for(int MeshIndex = 0; 
        MeshIndex < Model->Meshes.size(); 
        MeshIndex++)
    {
        added_asset Added = AddMeshAsset(System, &Model->Meshes[MeshIndex]);
        ToolModel->MeshIDs[MeshIndex] = Added.ID;
    }
    EndAsset(System);
    
    for(int MaterialIndex = 0;
        MaterialIndex < Model->Materials.size();
        MaterialIndex++)
    {
        loaded_mat* Material = &Model->Materials[MaterialIndex];
        Material->ToolMaterialInfo = LoadedToToolMaterialInfo(Material);
        tool_material_info* ToolMatInfo = &Material->ToolMaterialInfo;
        
        for(int TextureTypeIndex = 0;
            TextureTypeIndex < ArrayCount(SupportedTexturesTypes);
            TextureTypeIndex++)
        {
            // NOTE(Dima): Getting needed values
            int Count = Material->TextureCountOfType[TextureTypeIndex];
            int FirstIDInArray = Material->TextureFirstIndexOfTypeInArray[TextureTypeIndex];
            
            // NOTE(Dima): Adding bitmap arrays if needed
            BeginAsset(System, GameAsset_Type_BitmapArray);
            u32 AddedArrayID = 0;
            
            if(Count && (FirstIDInArray != -1)){
                // NOTE(Dima): Getting first bitmap ID for bitmap array
                // NOTE(Dima): A lot of lookups
                std::string FirstTypeTexturePath = Material->TexturePathArray[FirstIDInArray];
                loaded_mat_texture CorrespondingTexture = Context->PathToTextureMap[FirstTypeTexturePath];
                u32 FirstIDInBitmaps = CorrespondingTexture.StoredBitmapID;
                
                // NOTE(Dima): Adding bitmap array to asset system
                added_asset AddedArray = AddArrayAsset(System, FirstIDInBitmaps, Count);
                AddedArrayID = AddedArray.ID;
            }
            EndAsset(System);
            
            // NOTE(Dima): Setting corresponding array id in material info
            int OurTextureType = ConvertAssimpToOurTextureType(
                SupportedTexturesTypes[TextureTypeIndex]);
            ToolMatInfo->BitmapArrayIDs[OurTextureType] = AddedArrayID;
        }
        
        BeginAsset(System, GameAsset_Type_Material);
        added_asset AddedMaterial = AddMaterialAsset(System, ToolMatInfo);
        EndAsset(System);
        
        ToolModel->MaterialIDs[MaterialIndex] = AddedMaterial.ID;
    }
    
    BeginAsset(System, AssetGroupID);
    AddModelAsset(System, ToolModel);
    EndAsset(System);
}

INTERNAL_FUNCTION void StoreLoadingContext(asset_system* System, 
                                           model_loading_context* LoadingCtx)
{
    BeginAsset(System, GameAsset_Type_Bitmap);
    u32 FirstBmpID = 0;
    for(auto it: LoadingCtx->PathToTextureMap){
        added_asset AddedBitmap = AddBitmapAssetManual(System, &it.second.Bmp);
        it.second.StoredBitmapID = AddedBitmap.ID;
    }
    EndAsset(System);
    
    for(int SourceIndex = 0; 
        SourceIndex < LoadingCtx->ModelSources.size();
        SourceIndex++)
    {
        load_model_source* Source = &LoadingCtx->ModelSources[SourceIndex];
        
        Source->LoadedModel = LoadModelByASSIMP(Source->Path,
                                                Source->Flags,
                                                LoadingCtx);
        
        StoreModelAsset(System, 
                        Source->AssetGroup, 
                        &Source->LoadedModel,
                        LoadingCtx);
    }
}

inline void AddModelSource(model_loading_context* Ctx, load_model_source Source){
    Ctx->ModelSources.push_back(Source);
}

INTERNAL_FUNCTION void WriteMeshes1(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    model_loading_context LoadCtx = {};
    model_loading_context* Ctx = &LoadCtx;
    
    u32 DefaultFlags = 
        AssimpLoadMesh_GenerateNormals |
        AssimpLoadMesh_GenerateTangents;
    
    // NOTE(Dima): Here load all models
    AddModelSource(Ctx, ModelSource("../Data/Models/Bathroom/Bathroom.fbx", 
                                    GameAsset_Bathroom, 
                                    DefaultFlags));
    AddModelSource(Ctx, ModelSource("../Data/Models/Heart/Heart.fbx", 
                                    GameAsset_Heart, 
                                    DefaultFlags));
    AddModelSource(Ctx, ModelSource("../Data/Models/KindPlane/KindPlane.fbx", 
                                    GameAsset_KindPlane, 
                                    DefaultFlags));
    AddModelSource(Ctx, ModelSource("../Data/Models/Podkova/Podkova.fbx", 
                                    GameAsset_Podkova, 
                                    DefaultFlags));
    AddModelSource(Ctx, ModelSource("../Data/Models/RubbishBinBig/RubbishBin.fbx", 
                                    GameAsset_RubbishBin, 
                                    DefaultFlags));
    AddModelSource(Ctx, ModelSource("../Data/Models/Snowman/snowman.fbx", 
                                    GameAsset_Snowman, 
                                    DefaultFlags));
    AddModelSource(Ctx, ModelSource("../Data/Models/Stool/stool.fbx", 
                                    GameAsset_Stool, 
                                    DefaultFlags));
    AddModelSource(Ctx, ModelSource("../Data/Models/Toilet/toilet.fbx", 
                                    GameAsset_Toilet, 
                                    DefaultFlags));
    AddModelSource(Ctx, ModelSource("../Data/Models/Vaza/vaza1.fbx", 
                                    GameAsset_Vase, 
                                    DefaultFlags));
    
    // NOTE(Dima): Storing loading context
    StoreLoadingContext(System, Ctx);
    
    WriteAssetFile(System, "../Data/AssimpMeshes1.ja");
}

int main(int ArgsCount, char** Args){
    
    WriteMeshes1();
    
    system("pause");
    return(0);
}