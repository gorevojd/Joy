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

INTERNAL_FUNCTION u32 
CalculateCheckSumForSkeleton(tool_skeleton_info* Skeleton)
{
    u32 Result = 0;
    
    for(int Index = 0;
        Index < Skeleton->Bones.size();
        Index++)
    {
        bone_info* Bone = &Skeleton->Bones[Index];
        
        u32 ThisNameHash = StringHashFNV(Bone->Name);
        
        Result += ThisNameHash * 23 * (Index + 1) + 7;
    }
    
    return(Result);
}

struct assimp_node_traverse_entry{
    aiNode* Node;
    int ParentIndex;
    m44 ToWorld;
};

struct assimp_skeleton_build_entry{
    int NodeOrBoneIndex;
    int ParentBoneIndex;
};

struct temp_bone_storage{
    int ParentIndex;
    int NodeIndex;
    
    std::vector<int> ChildStorageIndices;
};

loaded_model LoadModelByASSIMP(char* FileName, u32 Flags, model_loading_context* LoadingCtx)
{
    loaded_model Result = {};
    
	Assimp::Importer importer;
    
    // NOTE(Dima): Processing some flags
	b32 JoyShouldCalculateNormals = 0;
	b32 JoyShouldCalculateTangents = 0;
    
	u64 AssimpFlags = 
        aiProcess_OptimizeMeshes |
		aiProcess_OptimizeGraph |
        aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType;
    
    b32 InGenTangents = Flags & AssimpLoadMesh_GenerateTangents;
    b32 InGenNorm = Flags & AssimpLoadMesh_GenerateNormals;
    b32 InGenSmNorm = Flags & AssimpLoadMesh_GenerateSmoothNormals;
    b32 InImportOnlyAnimation = Flags & AssimpLoadMesh_ImportOnlyAnimation;
    
    // NOTE(Dima): Generating tangents if needed
	if (InGenTangents) {
		AssimpFlags |= aiProcess_CalcTangentSpace;
	}
	else {
		JoyShouldCalculateTangents = true;
	}
    
    // NOTE(Dima): Generating normals if needed
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
    
    // NOTE(Dima): Reading scene
	const aiScene* scene = importer.ReadFile(
		FileName,
		AssimpFlags);
    
	if (scene) {
		printf("Loaded by ASSIMP: %s\n", FileName);
	}
	else {
		printf("Error reading ASSIMP: %s\n", FileName);
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
    
    // NOTE(Dima): Reading node hierarchy BFS and ordering it
    assimp_node_traverse_entry FirstEntry = {};
    FirstEntry.Node = scene->mRootNode;
    FirstEntry.ParentIndex = -1;
    FirstEntry.ToWorld = Assimp2JoyMatrix(scene->mRootNode->mTransformation);
    std::queue<assimp_node_traverse_entry> NodeProcessQueue;
    NodeProcessQueue.push(FirstEntry);
    
    int EntriesPushedTotal = 1;
    
    while(!NodeProcessQueue.empty()){
        
        assimp_node_traverse_entry& CurEntry = NodeProcessQueue.front();
        
        aiNode* CurNode = CurEntry.Node;
        
        loaded_node* ParentNode = 0;
        if(CurEntry.ParentIndex != -1){
            ParentNode = &Result.Nodes[CurEntry.ParentIndex];
        }
        
        loaded_node NewNode;
        NewNode.ChildCount = CurNode->mNumChildren;
        NewNode.FirstChildIndex = EntriesPushedTotal;
        NewNode.ParentIndex = CurEntry.ParentIndex;
        NewNode.AssimpNode = CurNode;
        NewNode.AssimpBone = 0;
        NewNode.ToParent = Assimp2JoyMatrix(CurNode->mTransformation);
        NewNode.ToWorld = NewNode.ToParent * CurEntry.ToWorld;
        strcpy(NewNode.Name, CurNode->mName.C_Str());
        
        int ThisNodeIndex = Result.Nodes.size();
        
        Result.Nodes.push_back(NewNode);
        
        for(int ChildIndex = 0;
            ChildIndex < CurNode->mNumChildren;
            ChildIndex++)
        {
            assimp_node_traverse_entry Entry = {};
            Entry.Node = CurNode->mChildren[ChildIndex];
            Entry.ParentIndex = ThisNodeIndex;
            
            NodeProcessQueue.push(Entry);
            
            ++EntriesPushedTotal;
        }
        
        // NOTE(Dima): Removing process element
        NodeProcessQueue.pop();
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
			v3 N = Assimp2JoyVector3(AssimpMesh->mNormals[VIndex]);
            
			Vertices.push_back(P);
			Normals.push_back(N);
            
            /*
            NOTE(dima): For some reason ASSIMP sometimes fails to generate tangents.
            So here i will skip tangent storing if they were not generated.
            That way tangents will be generated by MakeMesh
            */
            if(AssimpMesh->mTangents){
                v3 T = Assimp2JoyVector3(AssimpMesh->mTangents[VIndex]);
                
                Tangents.push_back(T);
            }
        }
        
        // NOTE(Dima): Generating and pushing mesh if needed
        loaded_mesh_slot MeshSlot = {};
        
        MeshSlot.MeshLoaded = 0;
        if(!InImportOnlyAnimation){
            MeshSlot.Mesh = MakeMesh(
                Vertices,
                TexCoords,
                Normals,
                Tangents,
                Colors,
                Indices,
                JoyShouldCalculateNormals,
                JoyShouldCalculateTangents);
        }
        
        int ResultSkeletonIndex = -1;
        
        if(AssimpMesh->HasBones()){
            int BonesCount = AssimpMesh->mNumBones;
            
            std::unordered_set<std::string> BoneNames;
            
            // NOTE(Dima): Finding nodes corresponding to bones and storing these pairs
            for(int BoneIndex = 0;
                BoneIndex < BonesCount;
                BoneIndex++)
            {
                aiBone* AssimpBone = AssimpMesh->mBones[BoneIndex];
                
                BoneNames.insert(std::string(AssimpBone->mName.C_Str()));
                
                // NOTE(Dima): Finding corresponding Node in hierarchy
                b32 CorrespondingFound = false;
                
                for(int NodeIndex = 0;
                    NodeIndex < Result.Nodes.size();
                    NodeIndex++)
                {
                    loaded_node* OurNode = &Result.Nodes[NodeIndex];
                    
                    if(strcmp(AssimpBone->mName.C_Str(), OurNode->Name) == 0){
                        OurNode->AssimpBone = AssimpBone;
                        
                        Result.BoneNameToNode.insert({std::string(OurNode->Name), NodeIndex});
                        
                        CorrespondingFound = true;
                        
                        break;
                    }
                }
                
                // NOTE(Dima): Corresponding Node should be found
                ASSERT(CorrespondingFound);
            }
            
            // NOTE(Dima): Now just go through the hierarchy and store loaded skeleton bones
            int RootBoneNodeIndex = -1;
            
            /*
            NOTE(Dima): Nodes are organized in tree like structure so first found node 
            with the name that exist in BoneNames will be our root node of 
            our skeleton!
            */
            for(int ScanIndex = 0; 
                ScanIndex < Result.Nodes.size();
                ScanIndex++)
            {
                loaded_node* CurNode = &Result.Nodes[ScanIndex];
                
                if(BoneNames.find(std::string(CurNode->Name)) != BoneNames.end()){
                    RootBoneNodeIndex = ScanIndex;
                    break;
                }
            }
            
            // NOTE(Dima): Root bone node should be found anyway
            ASSERT(RootBoneNodeIndex != -1);
            
            
            /*
            
            NOTE(dima): Here I build temp bones hierarchy. It represented as
            array of temp_bone_storage structures; Each object of this structure
            has an index of parent and vector of child indices; I begin walking
            down the hierarchy BFS (to save the hierarchy); If i meet node with 
            a name of one of the bones than i initialize it and add new child index
            to that bone's parent.
            
            */
            assimp_skeleton_build_entry FirstEntry;
            FirstEntry.NodeOrBoneIndex = RootBoneNodeIndex;
            FirstEntry.ParentBoneIndex = -1;
            
            std::queue<assimp_skeleton_build_entry> ScanQueue;
            ScanQueue.push(FirstEntry);
            
            std::vector<temp_bone_storage> TempBonesHierarchy;
            
            while(!ScanQueue.empty()){
                assimp_skeleton_build_entry& BuildEntry = ScanQueue.front();
                int CurrentIndex = BuildEntry.NodeOrBoneIndex;
                
                loaded_node* CurNode = &Result.Nodes[CurrentIndex];
                
                /*
NOTE(Dima): If this is bone than init it and add 
               new child index to parent (if that parent exist of course 
            in case of root bone);
               */
                
                int NewParentIndex = BuildEntry.ParentBoneIndex;
                if(BoneNames.find(std::string(CurNode->Name)) != BoneNames.end()){
                    
                    temp_bone_storage TempBone;
                    TempBone.ParentIndex = NewParentIndex;
                    TempBone.NodeIndex = CurrentIndex;
                    
                    int NewBoneStorageIndex = TempBonesHierarchy.size();
                    if(NewParentIndex != -1){
                        temp_bone_storage* ParentBone = &TempBonesHierarchy[NewParentIndex];
                        
                        ParentBone->ChildStorageIndices.push_back(NewBoneStorageIndex);
                    }
                    TempBonesHierarchy.push_back(TempBone);
                    
                    NewParentIndex = NewBoneStorageIndex;
                }
                
                // NOTE(Dima): Pushing childrent to the building queue
                int ChildStartIndex = CurNode->FirstChildIndex;
                int OnePastLastIndex = ChildStartIndex + CurNode->ChildCount;
                
                for(int ChildIndex = ChildStartIndex;
                    ChildIndex < OnePastLastIndex;
                    ChildIndex++)
                {
                    assimp_skeleton_build_entry NewEntry;
                    
                    NewEntry.NodeOrBoneIndex = ChildIndex;
                    NewEntry.ParentBoneIndex = NewParentIndex;
                    
                    ScanQueue.push(NewEntry);
                }
                
                ScanQueue.pop();
            }
            
            // NOTE(Dima): Temp bones hierarchy should be same size as bones
            ASSERT(BoneNames.size() == TempBonesHierarchy.size());
            
            /*
            NOTE (Dima): Now when i have temp bones hierarchy I can just store 
            it in skeleton_bones array. For this purpose third (and hopefully last) 
BFS loop will be created for walking through this temp hierarchy and 
just setting needed data;
            */
            tool_skeleton_info Skeleton;
            
            FirstEntry.NodeOrBoneIndex = 0;
            FirstEntry.ParentBoneIndex = -1;
            
            std::queue<assimp_skeleton_build_entry> FinalQueue;
            FinalQueue.push(FirstEntry);
            
            EntriesPushedTotal = 1;
            
            while(!FinalQueue.empty()){
                
                assimp_skeleton_build_entry& CurrentFront = FinalQueue.front();
                int TempBoneIndex = CurrentFront.NodeOrBoneIndex;
                temp_bone_storage* TempBone = &TempBonesHierarchy[TempBoneIndex];
                
                std::vector<int>& BoneChildren = TempBone->ChildStorageIndices;
                loaded_node* CurNode = &Result.Nodes[TempBone->NodeIndex];
                
                
                // NOTE(Dima): Init new bone
                bone_info NewBone;
                
                NewBone.ParentIndex = CurrentFront.ParentBoneIndex;
                NewBone.FirstChildIndex = EntriesPushedTotal;
                NewBone.ChildCount = BoneChildren.size();
                CopyStringsSafe(NewBone.Name, ARRAY_COUNT(NewBone.Name), CurNode->Name);
                NewBone.InvBindPose = Assimp2JoyMatrix(CurNode->AssimpBone->mOffsetMatrix);
                
                int ThisBoneIndex = Skeleton.Bones.size();
                
                // NOTE(Dima): Pushing bone to skeleton
                Skeleton.Bones.push_back(NewBone);
                
                for(int ChildIndex = 0;
                    ChildIndex < BoneChildren.size();
                    ChildIndex++)
                {
                    int StorageIndex = BoneChildren[ChildIndex];
                    
                    assimp_skeleton_build_entry NewEntry;
                    NewEntry.NodeOrBoneIndex = StorageIndex;
                    NewEntry.ParentBoneIndex = ThisBoneIndex;
                    
                    FinalQueue.push(NewEntry);
                    
                    ++EntriesPushedTotal;
                }
                
                FinalQueue.pop();
            }
            
            // NOTE(Dima): Trying to find skeleton with same check sum in skeletons array
            Skeleton.CheckSum = CalculateCheckSumForSkeleton(&Skeleton);
            
            for(int SkeletonIndex = 0;
                SkeletonIndex < Result.Skeletons.size();
                SkeletonIndex++)
            {
                tool_skeleton_info* CurSkeleton = &Result.Skeletons[SkeletonIndex];
                
                if(CurSkeleton->CheckSum == Skeleton.CheckSum){
                    ResultSkeletonIndex = SkeletonIndex;
                }
            }
            
            // NOTE(Dima): If there were no skeleton with same checksum then push skeleton
            if(ResultSkeletonIndex == -1){
                ResultSkeletonIndex = Result.Skeletons.size();
                Result.Skeletons.push_back(Skeleton);
            }
        }
        
        MeshSlot.SkeletonIndex = ResultSkeletonIndex;
        
        Result.Meshes.push_back(MeshSlot);
    }
    
    return(Result);
}

INTERNAL_FUNCTION tool_model_info LoadedToToolModelInfo(loaded_model* Model){
    tool_model_info Result = {};
    
    Result.MeshCount = Model->Meshes.size();
    Result.MaterialCount = Model->Materials.size();
    Result.SkeletonCount = Model->Skeletons.size();
    
    Result.MeshIDs = std::vector<u32>();
    Result.MaterialIDs = std::vector<u32>();
    Result.SkeletonIDs = std::vector<u32>();
    
    for(auto &Node: Model->Nodes){
        node_info NewNode;
        
        CopyStringsSafe(NewNode.Name, ARRAY_COUNT(NewNode.Name), Node.Name);
        NewNode.ToParent = Node.ToParent;
        NewNode.ToWorld = Node.ToWorld;
        NewNode.ParentIndex = Node.ParentIndex;
        NewNode.FirstChildIndex = Node.FirstChildIndex;
        NewNode.ChildCount = Node.ChildCount;
        
        Result.Nodes.push_back(NewNode);
    }
    
    ASSERT(Result.Nodes.size() == Model->Nodes.size());
    
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
        added_asset Added = AddMeshAsset(System, &Model->Meshes[MeshIndex].Mesh);
        ToolModel->MeshIDs.push_back(Added.ID);
    }
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Type_Skeleton);
    for(int SkeletonIndex = 0;
        SkeletonIndex < Model->Skeletons.size();
        SkeletonIndex++)
    {
        added_asset Added = AddSkeletonAsset(System, &Model->Skeletons[SkeletonIndex]);
        ToolModel->SkeletonIDs.push_back(Added.ID);
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
        
        ToolModel->MaterialIDs.push_back(AddedMaterial.ID);
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
    
    AddModelSource(Ctx, ModelSource("../Data/Models/Animations/Male_Casual.fbx",
                                    GameAsset_Man,
                                    DefaultFlags));
    
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