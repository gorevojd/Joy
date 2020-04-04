#include "tool_asset_build_assimp.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"

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
        
        mat_texture_source MatTexSource = {};
        
        const char* TexturePathSrc = TexturePath.C_Str(); 
        if(TexturePathSrc[0] == '*'){
            // NOTE(Dima): This is embeded texture index
            char EmbedIndexString[64];
            for(int i = 1; i < 64; i++){
                
                char NewChar = TexturePathSrc[i];
                EmbedIndexString[i - 1] = NewChar;
                
                if(NewChar == 0){
                    break;
                }
            }
            
            int EmbedIndex = StringToInteger(EmbedIndexString);
            
            MatTexSource.IsEmbeded = true;
            MatTexSource.EmbedIndex = EmbedIndex;
        }
        else{
            // NOTE(Dima): This is normal texture path
            std::string TexturePathNew(TexturePath.C_Str());
            
            // NOTE(Dima): Trying to find texture in embed mapping
            // NOTE(Dima): If it exist there - than it's embeded texture
            auto& TexNameToEmbedIndex = Model->TextureNameToEmbedIndex;
            auto FindIt = TexNameToEmbedIndex.find(TexturePathNew);
            
            b32 IsEmbedTexture = (FindIt != TexNameToEmbedIndex.end());
            
            if(IsEmbedTexture){
                int EmbedIndex = TexNameToEmbedIndex[TexturePathNew];
                
                MatTexSource.IsEmbeded = true;
                MatTexSource.EmbedIndex = EmbedIndex;
            }
            else{
                auto* Mapping = &Ctx->PathToTextureMap;
                
                b32 NotLoaded = (Mapping->find(TexturePathNew) == Mapping->end());
                if(NotLoaded){
                    loaded_mat_texture Tex;
                    Tex.Bmp = LoadBMP((char*)TexturePathNew.c_str());
                    
                    Mapping->insert(std::make_pair(TexturePathNew, Tex));
                }
                
                MatTexSource.IsEmbeded = false;
                MatTexSource.Path = TexturePathNew;
            }
        }
        
        // NOTE(Dima): If this is the first texture of type 
        // NOTE(Dima): than return it
        if(TextureIndex == 0){
            Result.FirstID = OurMat->TextureSourceArray.size();
        }
        
        OurMat->TextureSourceArray.push_back(MatTexSource);
    }
    
    return(Result);
}

INTERNAL_FUNCTION u32 
CalculateCheckSumForSkeleton(loaded_model* Model, tool_skeleton_info* Skeleton)
{
    u32 Result = 0;
    
    for(int Index = 0;
        Index < Skeleton->Bones.size();
        Index++)
    {
        bone_info* Bone = &Skeleton->Bones[Index];
        
        loaded_node* Node = &Model->Nodes[Bone->NodeIndex];
        
        u32 ThisNameHash = StringHashFNV(Node->Name);
        
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
    
    // NOTE(Dima): Loading embeded textures
    for(int EmbedIndex = 0;
        EmbedIndex < scene->mNumTextures;
        EmbedIndex++)
    {
        aiTexture* EmbedTexture = scene->mTextures[EmbedIndex];
        
        b32 IsCompressed = EmbedTexture->mHeight == 0;
        
        // NOTE(Dima): If texture is uncompressed, this vector will store packed pixels
        std::vector<u32> OurColors;
        
        unsigned char* LoadFromData = 0;
        u32 DataSize = 0;
        if(IsCompressed){
            DataSize = EmbedTexture->mWidth;
            
            LoadFromData = (unsigned char*)EmbedTexture->pcData;
        }
        else{
            aiTexel* Texels = EmbedTexture->pcData;
            int TexelsCount = EmbedTexture->mWidth * EmbedTexture->mHeight;
            
            // NOTE(Dima): Packing pixels
            for(int TexelIndex = 0; 
                TexelIndex < TexelsCount;
                TexelIndex++)
            {
                aiTexel Texel = Texels[TexelIndex];
                
                aiColor4D AssimpColor = aiColor4D(Texel);
                
                v4 OurColor;
                OurColor.r = AssimpColor.r;
                OurColor.g = AssimpColor.g;
                OurColor.b = AssimpColor.b;
                OurColor.a = AssimpColor.a;
                
                u32 Packed = PackRGBA(OurColor);
                OurColors.push_back(Packed);
            }
            
            DataSize = TexelsCount * 4;
            LoadFromData = (unsigned char*)(&OurColors[0]);
        }
        
        loaded_mat_texture MatTexture;
        MatTexture.Bmp = LoadFromDataBMP(LoadFromData, DataSize);
        
        std::string Name = std::string(EmbedTexture->mFilename.C_Str());
        MatTexture.Name = Name;
        
        Result.TextureNameToEmbedIndex.insert({Name, EmbedIndex});
        
        Result.EmbededTextures.push_back(MatTexture);
    }
    
    //NOTE(Dima): Loading materials
    for(int MatIndex = 0; MatIndex < scene->mNumMaterials;MatIndex++){
        aiMaterial* AssimpMaterial = scene->mMaterials[MatIndex];
        loaded_mat NewMaterial = {};
        
        // NOTE(Dima): Getting material name
        aiString AssimpMatName;
        if(AssimpMaterial->Get(AI_MATKEY_NAME, AssimpMatName) == AI_SUCCESS){
            CopyStringsSafe(NewMaterial.Name, 
                            ARRAY_COUNT(NewMaterial.Name), 
                            (char*)AssimpMatName.C_Str());
        }
        
        v3 DiffuseColorVector = V3(1.0f, 1.0f, 1.0f);
        aiColor3D AssimpDiffuseColor;
        if(AssimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, AssimpDiffuseColor) == AI_SUCCESS){
            DiffuseColorVector = Assimp2JoyColor3(AssimpDiffuseColor);
        }
        
        v3 AmbientColorVector = V3(1.0f, 1.0f, 1.0f);
        aiColor3D AssimpAmbientColor;
        if(AssimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AssimpAmbientColor) == AI_SUCCESS){
            AmbientColorVector = Assimp2JoyColor3(AssimpAmbientColor);
        }
        
        v3 SpecularColorVector = V3(1.0f, 1.0f, 1.0f);
        aiColor3D AssimpSpecularColor;
        if(AssimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, AssimpSpecularColor) == AI_SUCCESS){
            SpecularColorVector = Assimp2JoyColor3(AssimpSpecularColor);
        }
        
        
        v3 EmissiveColorVector = V3(0.0f, 0.0f, 0.0f);
        aiColor3D AssimpEmissiveColor;
        if(AssimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, AssimpEmissiveColor) == AI_SUCCESS){
            EmissiveColorVector = Assimp2JoyColor3(AssimpEmissiveColor);
        }
        
        u32 DiffuseColor = PackRGB_R10G12B10(DiffuseColorVector);
        u32 AmbientColor = PackRGB_R10G12B10(AmbientColorVector);
        u32 SpecularColor = PackRGB_R10G12B10(SpecularColorVector);
        u32 EmissiveColor = PackRGB_R10G12B10(EmissiveColorVector);
        
        NewMaterial.ColorDiffusePacked = DiffuseColor;
        NewMaterial.ColorAmbientPacked = AmbientColor;
        NewMaterial.ColorSpecularPacked = SpecularColor;
        NewMaterial.ColorEmissivePacked = EmissiveColor;
        
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
    FirstEntry.ToWorld = Transpose(Assimp2JoyMatrix(scene->mRootNode->mTransformation));
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
        NewNode.ToParent = Transpose(Assimp2JoyMatrix(CurNode->mTransformation));
        NewNode.ToWorld = NewNode.ToParent * CurEntry.ToWorld;
        strcpy(NewNode.Name, CurNode->mName.C_Str());
        
        // NOTE(Dima): Inserting all child nodes
        for(int NodeMeshIndex = 0;
            NodeMeshIndex < CurNode->mNumMeshes;
            NodeMeshIndex++)
        {
            NewNode.MeshIndices.push_back(CurNode->mMeshes[NodeMeshIndex]);
        }
        
        int ThisNodeIndex = Result.Nodes.size();
        
        Result.NodeNameToNodeIndex.insert({std::string(NewNode.Name), ThisNodeIndex});
        Result.Nodes.push_back(NewNode);
        
        for(int ChildIndex = 0;
            ChildIndex < CurNode->mNumChildren;
            ChildIndex++)
        {
            assimp_node_traverse_entry Entry = {};
            Entry.Node = CurNode->mChildren[ChildIndex];
            Entry.ParentIndex = ThisNodeIndex;
            Entry.ToWorld = NewNode.ToWorld;
            
            NodeProcessQueue.push(Entry);
            
            ++EntriesPushedTotal;
        }
        
        // NOTE(Dima): Removing process element
        NodeProcessQueue.pop();
    }
    
    // NOTE(Dima): Loading nodes animations
    int AnimationsCount = scene->mNumAnimations;
    for(int AnimIndex = 0;
        AnimIndex < AnimationsCount;
        AnimIndex++)
    {
        aiAnimation* AssimpAnim = scene->mAnimations[AnimIndex];
        
        tool_animation_info NewAnimation;
        
        NewAnimation.Name = std::string(AssimpAnim->mName.C_Str());
        NewAnimation.Duration = AssimpAnim->mDuration;
        NewAnimation.TicksPerSecond = AssimpAnim->mTicksPerSecond;
        
        for(int NodeAnimIndex = 0;
            NodeAnimIndex < AssimpAnim->mNumChannels;
            NodeAnimIndex++)
        {
            aiNodeAnim* AssimpNodeAnim = AssimpAnim->mChannels[NodeAnimIndex];
            
            tool_node_animation NewNodeAnim = {};
            
            // NOTE(Dima): Setting name
            NewNodeAnim.NodeName = std::string(AssimpNodeAnim->mNodeName.C_Str());
            
            // NOTE(Dima): Finding node index in a mapping
            auto FindIterator = Result.NodeNameToNodeIndex.find(NewNodeAnim.NodeName);
            ASSERT(FindIterator != Result.NodeNameToNodeIndex.end());
            NewNodeAnim.NodeIndex = FindIterator->second;
            
            // NOTE(Dima): Loading position keys
            for(int KeyIndex = 0; 
                KeyIndex < AssimpNodeAnim->mNumPositionKeys; 
                KeyIndex++)
            {
                aiVectorKey* Key = &AssimpNodeAnim->mPositionKeys[KeyIndex];
                
                animation_vector_key NewKey = {};
                NewKey.Value = Assimp2JoyVector3(Key->mValue);
                NewKey.Time = Key->mTime;
                
                NewNodeAnim.PositionKeys.push_back(NewKey);
            }
            
            // NOTE(Dima): Loading rotation keys
            for(int KeyIndex = 0; 
                KeyIndex < AssimpNodeAnim->mNumRotationKeys; 
                KeyIndex++)
            {
                aiQuatKey* Key = &AssimpNodeAnim->mRotationKeys[KeyIndex];
                
                animation_quaternion_key NewKey = {};
                NewKey.Value = Assimp2JoyQuat(Key->mValue);
                NewKey.Time = Key->mTime;
                
                NewNodeAnim.RotationKeys.push_back(NewKey);
            }
            
            // NOTE(Dima): Loading scaling keys
            for(int KeyIndex = 0; 
                KeyIndex < AssimpNodeAnim->mNumScalingKeys; 
                KeyIndex++)
            {
                aiVectorKey* Key = &AssimpNodeAnim->mScalingKeys[KeyIndex];
                
                animation_vector_key NewKey = {};
                NewKey.Value = Assimp2JoyVector3(Key->mValue);
                NewKey.Time = Key->mTime;
                
                NewNodeAnim.ScalingKeys.push_back(NewKey);
            }
            
            // NOTE(Dima): Pushing node animation to vector
            NewAnimation.NodeAnimations.push_back(NewNodeAnim);
        }
        
        // NOTE(Dima): Pushing animation to vector
        Result.Animations.push_back(NewAnimation);
    }
    
    // NOTE(Dima): Loading meshes
    int NumMeshes = scene->mNumMeshes;
    for (int MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++) {
        aiMesh* AssimpMesh = scene->mMeshes[MeshIndex];
        
        int ResultSkeletonIndex = -1;
        int BonesCount = 0;
        
        if(AssimpMesh->HasBones()){
            BonesCount = AssimpMesh->mNumBones;
            
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
                        CorrespondingFound = true;
                        
                        OurNode->AssimpBone = AssimpBone;
                        
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
                aiMatrix4x4 BindPoseMatrix = CurNode->AssimpBone->mOffsetMatrix;
                BindPoseMatrix = BindPoseMatrix.Transpose();
                NewBone.InvBindPose = Assimp2JoyMatrix(BindPoseMatrix);
                NewBone.NodeIndex = TempBone->NodeIndex;
                
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
            
            // NOTE(Dima): Create Bone name to Bone index mapping
            for(int BoneIndex = 0;
                BoneIndex < Skeleton.Bones.size();
                BoneIndex++)
            {
                bone_info* Bone = &Skeleton.Bones[BoneIndex];
                
                loaded_node* Node = &Result.Nodes[Bone->NodeIndex];
                
                Skeleton.BoneNameToBoneID.insert({Node->Name, BoneIndex});
            }
            
            // NOTE(Dima): Trying to find skeleton with same check sum in skeletons array
            Skeleton.CheckSum = CalculateCheckSumForSkeleton(&Result, &Skeleton);
            
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
        
        // NOTE(Dima): Loading mesh data
        std::vector<v3> Vertices;
        std::vector<v3> Normals;
        std::vector<v2> TexCoords;
        std::vector<v3> Tangents;
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
        
        std::vector<vertex_weights> Weights;
        if(BonesCount){
            Weights.insert(Weights.begin(), AssimpMesh->mNumVertices, {});
            
            tool_skeleton_info* Skeleton = &Result.Skeletons[ResultSkeletonIndex];
            
            // NOTE(Dima): Iterating through bones and getting vertex weights
            for(int BoneIndex = 0;
                BoneIndex < BonesCount;
                BoneIndex++)
            {
                aiBone* AssimpBone = AssimpMesh->mBones[BoneIndex];
                
                auto FindBoneIDIt = Skeleton->BoneNameToBoneID.find(std::string(AssimpBone->mName.C_Str()));
                
                // NOTE(Dima): Result should be found
                ASSERT(FindBoneIDIt != Skeleton->BoneNameToBoneID.end());
                
                int BoneID = FindBoneIDIt->second;
                
                for(int VertexWeightIndex = 0;
                    VertexWeightIndex < AssimpBone->mNumWeights;
                    VertexWeightIndex++)
                {
                    aiVertexWeight* AssimpWeight = &AssimpBone->mWeights[VertexWeightIndex];
                    
                    vertex_weights* Target = &Weights[AssimpWeight->mVertexId];
                    
                    vertex_weight NewWeight;
                    
                    float Weight = AssimpWeight->mWeight;
                    NewWeight.BoneID = BoneID;
                    NewWeight.Weight = Weight;
                    
                    Target->Weights.push_back(NewWeight);
                }
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
                Indices,
                Weights,
                JoyShouldCalculateNormals,
                JoyShouldCalculateTangents);
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
    Result.AnimationCount = Model->Animations.size();
    
    Result.MeshIDs = std::vector<u32>();
    Result.MaterialIDs = std::vector<u32>();
    Result.SkeletonIDs = std::vector<u32>();
    Result.AnimationIDs = std::vector<u32>();
    
    for(int NodeIndex = 0;
        NodeIndex < Model->Nodes.size();
        NodeIndex++)
    {
        loaded_node* Node = &Model->Nodes[NodeIndex];
        tool_node_info NewNode;
        
        CopyStringsSafe(NewNode.Shared.Name, ARRAY_COUNT(NewNode.Shared.Name), Node->Name);
        NewNode.Shared.ToParent = Node->ToParent;
        NewNode.Shared.ParentIndex = Node->ParentIndex;
        NewNode.Shared.FirstChildIndex = Node->FirstChildIndex;
        NewNode.Shared.ChildCount = Node->ChildCount;
        NewNode.MeshIndices = Node->MeshIndices;
        
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
    
    // NOTE(Dima): Storing embeded textures
    BeginAsset(System, GameAsset_Type_Bitmap);
    for(int EmbedIndex = 0;
        EmbedIndex < Model->EmbededTextures.size();
        EmbedIndex++)
    {
        loaded_mat_texture* MatTex = &Model->EmbededTextures[EmbedIndex];
        
        added_asset Added = AddBitmapAssetManual(System, &MatTex->Bmp);
        MatTex->StoredBitmapID = Added.ID;
    }
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Type_NodeAnim);
    for(int AnimIndex = 0;
        AnimIndex < Model->Animations.size();
        AnimIndex++)
    {
        tool_animation_info* Animation = &Model->Animations[AnimIndex];
        
        for(int NodeAnimIndex = 0;
            NodeAnimIndex < Animation->NodeAnimations.size();
            NodeAnimIndex++)
        {
            tool_node_animation* NodeAnim = &Animation->NodeAnimations[NodeAnimIndex];
            
            added_asset Added = AddNodeAnimationAsset(System, NodeAnim);
            Animation->NodeAnimationsStoredIDs.push_back(Added.ID);
        }
    }
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Type_AnimationClip);
    for(int AnimIndex = 0;
        AnimIndex < Model->Animations.size();
        AnimIndex++)
    {
        tool_animation_info* Animation = &Model->Animations[AnimIndex];
        
        CopyStringsSafe(Animation->StoreName, 
                        sizeof(Animation->StoreName),
                        (char*)Animation->Name.c_str());
        
        added_asset Added = AddAnimationClipAsset(System, Animation);
        ToolModel->AnimationIDs.push_back(Added.ID);
    }
    ASSERT(ToolModel->AnimationCount == ToolModel->AnimationIDs.size());
    EndAsset(System);
    
    // NOTE(Dima): Storing meshes
    BeginAsset(System, GameAsset_Type_Mesh);
    for(int MeshIndex = 0; 
        MeshIndex < Model->Meshes.size(); 
        MeshIndex++)
    {
        loaded_mesh_slot* Slot = &Model->Meshes[MeshIndex];
        
        added_asset Added = AddMeshAsset(System, &Slot->Mesh);
        Slot->StoredMeshID = Added.ID;
        ToolModel->MeshIDs.push_back(Added.ID);
    }
    EndAsset(System);
    
    // NOTE(Dima): Storing nodes
    for(int NodeIndex = 0;
        NodeIndex < ToolModel->Nodes.size();
        NodeIndex++)
    {
        tool_node_info* Node = &ToolModel->Nodes[NodeIndex];
        
        for(int NodeMeshIndex = 0;
            NodeMeshIndex < Node->MeshIndices.size();
            NodeMeshIndex++)
        {
            int MeshIndexInArray = Node->MeshIndices[NodeMeshIndex];
            
            u32 StoredMeshID = Model->Meshes[MeshIndexInArray].StoredMeshID;
            Node->MeshIDs.push_back(StoredMeshID);
        }
        
        auto InsertedIt = ToolModel->NodeMeshIndicesStorage.insert(
            ToolModel->NodeMeshIndicesStorage.end(),
            Node->MeshIDs.begin(),
            Node->MeshIDs.end());
        
        Node->Shared.NodeMeshIndexCountInStorage = Node->MeshIDs.size();
        Node->Shared.NodeMeshIndexFirstInStorage = std::distance(
            ToolModel->NodeMeshIndicesStorage.begin(),
            InsertedIt);
        
        ToolModel->NodesSharedDatas.push_back(Node->Shared);
    }
    
    
    // NOTE(Dima): Storing skeleton
    BeginAsset(System, GameAsset_Type_Skeleton);
    for(int SkeletonIndex = 0;
        SkeletonIndex < Model->Skeletons.size();
        SkeletonIndex++)
    {
        added_asset Added = AddSkeletonAsset(System, &Model->Skeletons[SkeletonIndex]);
        ToolModel->SkeletonIDs.push_back(Added.ID);
    }
    EndAsset(System);
    
    // NOTE(Dima): Storing materials
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
                loaded_mat_texture* CorrespondingTexture = 0;
                mat_texture_source* TexSource = &Material->TextureSourceArray[FirstIDInArray];
                if(TexSource->IsEmbeded){
                    CorrespondingTexture = &Model->EmbededTextures[TexSource->EmbedIndex];
                }
                else{
                    std::string FirstTypeTexturePath = TexSource->Path;
                    CorrespondingTexture = &Context->PathToTextureMap[FirstTypeTexturePath];
                }
                
                u32 FirstIDInBitmaps = CorrespondingTexture->StoredBitmapID;
                
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
    
    // NOTE(Dima): Storing actual model
    BeginAsset(System, AssetGroupID);
    AddModelAsset(System, ToolModel);
    EndAsset(System);
}

INTERNAL_FUNCTION void StoreLoadingContext(asset_system* System, 
                                           model_loading_context* LoadingCtx)
{
    BeginAsset(System, GameAsset_Type_Bitmap);
    // NOTE(Dima): Storing textures from loading context
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
    AddModelSource(Ctx, ModelSource("../Data/Models/Animations/test.fbx",
                                    GameAsset_Test,
                                    DefaultFlags));
    
    AddModelSource(Ctx, ModelSource("../Data/Models/Animations/spider.fbx",
                                    GameAsset_Spider,
                                    DefaultFlags));
    
    AddModelSource(Ctx, ModelSource("../Data/Models/Animations/Male_Casual.fbx",
                                    GameAsset_Man,
                                    DefaultFlags));
    
    AddModelSource(Ctx, ModelSource("../Data/Models/Skyscraper/Skyscraper.fbx", 
                                    GameAsset_Skyscraper,
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