#include "tool_asset_build_assimp.h"

#include "tool_asset_build_loading.cpp"
#include "tool_asset_build_commands.cpp"

inline aiTextureType ConvertOurToAssimp(u32 Our)
{
    aiTextureType Result = aiTextureType_UNKNOWN;
    
    switch(Our){
        case MaterialChannel_Albedo:{
            Result = aiTextureType_DIFFUSE;
        }break;
        
        case MaterialChannel_Specular:{
            Result = aiTextureType_SPECULAR;
        }break;
        
        case MaterialChannel_Ambient:{
            Result = aiTextureType_AMBIENT;
        }break;
        
        case MaterialChannel_Emissive:{
            Result = aiTextureType_EMISSIVE;
        }break;
        
        case MaterialChannel_Normal:{
            Result = aiTextureType_NORMALS;
        }break;
        
        case MaterialChannel_Metal:
        {
            Result = aiTextureType_SPECULAR;
        }break;
        
        case MaterialChannel_Roughness:
        {
            Result = aiTextureType_SPECULAR;
        }break;
    }
    
    return(Result);
}

void ReplaceSlashes(std::string& Path){
    for(int i = 0; i < Path.length(); i++){
        if(Path[i] == '\\'){
            Path[i] = '/';
        }
    }
}

void ReplaceSpecialPath(std::string& Path,
                        char* SpecialPath,
                        char* NewPath)
{
    // NOTE(Dima): if is special path - replace with what I need
    size_t SpecialFindIndex = Path.find(SpecialPath);
    b32 IsSpecial = (SpecialFindIndex != std::string::npos);
    
    if(IsSpecial){
        Path.replace(SpecialFindIndex, StringLength(SpecialPath), NewPath);
    }
}

INTERNAL_FUNCTION u32
LoadTextureForAssimpType(loaded_model* Model,
                         loaded_mat* OurMat,
                         model_loading_context* Ctx,
                         aiMaterial* Mat,
                         aiTextureType Type)
{
    u32 Result = -1;
    
    int TextureCountForType = Mat->GetTextureCount(Type);
    
    for(int TextureIndex = 0;
        TextureIndex < Min(TextureCountForType, 1);
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
                    ReplaceSpecialPath(TexturePathNew, 
                                       "..\\..\\..\\..\\",
                                       "..\\");
                    
                    ReplaceSpecialPath(TexturePathNew, 
                                       "..\\..\\..\\",
                                       "..\\");
                    
                    ReplaceSpecialPath(TexturePathNew, 
                                       "..\\..\\",
                                       "..\\");
                    
                    ReplaceSpecialPath(TexturePathNew, 
                                       "..\\Forest Animals Unity Project\\Forest Animals Unity\\Assets\\Forest Animals\\",
                                       "../Data/Models/ForestAnimals/");
                    
                    ReplaceSpecialPath(TexturePathNew, 
                                       "..\\Farm Animals Unity Project\\Assets\\Farm Animals\\",
                                       "../Data/Models/FarmAnimals/");
                    
                    ReplaceSpecialPath(TexturePathNew, 
                                       "..\\Assets\\Insect Characters\\",
                                       "../Data/Models/Insects/");
                    
                    ReplaceSlashes(TexturePathNew);
                    
                    loaded_mat_texture Tex;
                    Tex.Bmp = LoadBMP((char*)TexturePathNew.c_str());
                    Tex.Name = TexturePathNew;
                    
                    Mapping->insert(std::make_pair(TexturePathNew, Tex));
                }
                
                MatTexSource.IsEmbeded = false;
                MatTexSource.Path = TexturePathNew;
            }
        }
        
        // NOTE(Dima): If this is the first texture of type 
        // NOTE(Dima): than return it
        Result = OurMat->TextureSourceArray.size();
        
        OurMat->TextureSourceArray.push_back(MatTexSource);
    }
    
    return(Result);
}

INTERNAL_FUNCTION u32 
CalculateCheckSumForNodes(loaded_model* Model)
{
    u32 Result = 0;
    
    for(int Index = 0;
        Index < Model->Nodes.size();
        Index++)
    {
        loaded_node* Node  = &Model->Nodes[Index];
        
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

loaded_model LoadModelByASSIMP(char* FileName, u32 Flags, 
                               model_loading_context* LoadingCtx,
                               const std::string& RootMotionNodeName)
{
    loaded_model Result = {};
    
    Assimp::Importer importer;
    
    // NOTE(Dima): Processing some flags
    b32 JoyShouldCalculateNormals = 0;
    b32 JoyShouldCalculateTangents = 0;
    
    u64 AssimpFlags = 
#if 0        
    aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph |
#endif
    aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType;
    
    b32 InGenTangents = Flags & Load_GenerateTangents;
    b32 InGenNorm = Flags & Load_GenerateNormals;
    b32 InGenSmNorm = Flags & Load_GenerateSmoothNormals;
    b32 IsOnlyAnim = Flags & Load_ImportOnlyAnimation;
    b32 ExtractRootMotionY = (Flags & Load_ExtractRootMotionY) != 0;
    b32 ExtractRootMotionZ = (Flags & Load_ExtractRootMotionZ) != 0;
    b32 UsesRootMotion = ExtractRootMotionY || ExtractRootMotionZ;
    
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
    
    //scene->mMetaData->Get("UnitScaleFactor", factor);
    
    //double factor(0.0);
    //scene->mMetaData->Get("UnitScaleFactor", factor);
    //scene->mMetaData->Set("UnitScaleFactor", 100);
    
    if (scene) {
        printf("Loaded by ASSIMP: %s\n", FileName);
    }
    else {
        printf("Error reading ASSIMP: %s\n", FileName);
    }
    
    if(!IsOnlyAnim){
        
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
            
            NewMaterial.ColorsPacked[MaterialChannel_Albedo] = PackRGB_R10G12B10(DiffuseColorVector);
            NewMaterial.ColorsPacked[MaterialChannel_Ambient] = PackRGB_R10G12B10(AmbientColorVector);
            NewMaterial.ColorsPacked[MaterialChannel_Specular] = PackRGB_R10G12B10(SpecularColorVector);
            NewMaterial.ColorsPacked[MaterialChannel_Emissive] = PackRGB_R10G12B10(EmissiveColorVector);
            NewMaterial.ColorsPacked[MaterialChannel_Metal] = PackRGB_R10G12B10(V3_Zero());
            NewMaterial.ColorsPacked[MaterialChannel_Roughness] = PackRGB_R10G12B10(V3_One());
            NewMaterial.ColorsPacked[MaterialChannel_Normal] = PackRGB_R10G12B10(V3(0.0f, 0.0f, 1.0f));
            
            // NOTE(Dima): Loading textures for supported Assimp textures types
            for(int TTypeIndex = 0;
                TTypeIndex < MaterialChannel_Count;
                TTypeIndex++)
            {
                u32 TextureIndex = LoadTextureForAssimpType(&Result, 
                                                            &NewMaterial,
                                                            LoadingCtx, 
                                                            AssimpMaterial,
                                                            ConvertOurToAssimp(TTypeIndex));
                
                NewMaterial.TextureIndices[TTypeIndex] = TextureIndex;
            }
            
            Result.Materials.push_back(NewMaterial);
        }
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
    
    // NOTE(Dima): Calculating check sums
    Result.NodesCheckSum = CalculateCheckSumForNodes(&Result);
    
    std::unordered_set<std::string> BoneNames;
    std::unordered_map<std::string, int> BoneNameToBoneIndex;
    
    tool_skeleton_info* Skeleton = &Result.Skeleton;
    int NumMeshes = scene->mNumMeshes;
    
    // NOTE(Dima): Loading meshes bone data
    for (int MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++) {
        aiMesh* AssimpMesh = scene->mMeshes[MeshIndex];
        
        if(AssimpMesh->HasBones()){
            // NOTE(Dima): Finding nodes corresponding to bones and storing these pairs
            for(int BoneIndex = 0;
                BoneIndex < AssimpMesh->mNumBones;
                BoneIndex++)
            {
                aiBone* AssimpBone = AssimpMesh->mBones[BoneIndex];
                
                std::string BoneName(AssimpBone->mName.C_Str());
                if(BoneNames.find(BoneName) == BoneNames.end()){
                    // NOTE(Dima): Finding corresponding node
                    auto FindNodeIt = Result.NodeNameToNodeIndex.find(BoneName);
                    Assert(FindNodeIt != Result.NodeNameToNodeIndex.end());
                    
                    int BoneNodeIndex = FindNodeIt->second;
                    
                    loaded_node* CurNode = &Result.Nodes[BoneNodeIndex];
                    CurNode->AssimpBone = AssimpBone;
                    
                    // NOTE(Dima): Creating new bone
                    bone_info NewBone;
                    
                    aiMatrix4x4 InvBindPoseMatrix = AssimpBone->mOffsetMatrix;
                    InvBindPoseMatrix = InvBindPoseMatrix.Transpose();
                    NewBone.InvBindPose = Assimp2JoyMatrix(InvBindPoseMatrix);
                    NewBone.NodeIndex = BoneNodeIndex;
                    NewBone.ParentIndex = -1;
                    
                    int ThisBoneIndex = Skeleton->Bones.size();
                    
                    // NOTE(Dima): Pushing bone to skeleton
                    Skeleton->Bones.push_back(NewBone);
                    
                    BoneNameToBoneIndex.insert({std::string(AssimpBone->mName.C_Str()), ThisBoneIndex});
                    BoneNames.insert(std::string(AssimpBone->mName.C_Str()));
                }
            }
        }
    }
    
    
    Result.HasSkeleton = Skeleton->Bones.size() > 0;
    int RootBoneNodeIndex = -1;
    
    if(Result.HasSkeleton){
        // NOTE(Dima): Now just go through the hierarchy and store loaded skeleton bones
        
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
        
        // NOTE(Dima): Finding and setting bones parents
        for(int BoneIndex = 0;
            BoneIndex < Skeleton->Bones.size();
            BoneIndex++)
        {
            bone_info* Bone = &Skeleton->Bones[BoneIndex];
            
            if(Bone->NodeIndex == RootBoneNodeIndex){
                Bone->ParentIndex = -1;
            }
            else{
                int ParentBoneIndex = -1;
                b32 ParentBoneFound = false;
                int ThisNodeIndex = Result.Nodes[Bone->NodeIndex].ParentIndex;
                while(ThisNodeIndex != -1){
                    loaded_node* Node = &Result.Nodes[ThisNodeIndex];
                    
                    auto FindBoneIt = BoneNameToBoneIndex.find(std::string(Node->Name));
                    
                    if(FindBoneIt != BoneNameToBoneIndex.end()){
                        ParentBoneIndex = FindBoneIt->second;
                        
                        ParentBoneFound = true;
                        break;
                    }
                    
                    ThisNodeIndex = Node->ParentIndex;
                }
                
                Assert(ParentBoneFound);
                Bone->ParentIndex = ParentBoneIndex;
            }
        }
    }
    
    // NOTE(Dima): Loading nodes animations
    int AnimationsCount = scene->mNumAnimations;
    
    for(int AnimIndex = 0;
        AnimIndex < AnimationsCount;
        AnimIndex++)
    {
        aiAnimation* AssimpAnim = scene->mAnimations[AnimIndex];
        
        // NOTE(Dima): Getting animated node names
        std::set<std::string> AnimatedNodeNames;
        for(int NodeAnimIndex = 0;
            NodeAnimIndex < AssimpAnim->mNumChannels;
            NodeAnimIndex++)
        {
            aiNodeAnim* AssimpNodeAnim = AssimpAnim->mChannels[NodeAnimIndex];
            
            std::string NodeName = std::string(AssimpNodeAnim->mNodeName.C_Str());
            AnimatedNodeNames.insert(NodeName);
        }
        
        // NOTE(Dima): Finding root animated node name
        std::string RootAnimatedNodeName = "";
        if(RootMotionNodeName == ""){
            for(int ScanIndex = 0; 
                ScanIndex < Result.Nodes.size();
                ScanIndex++)
            {
                loaded_node* CurNode = &Result.Nodes[ScanIndex];
                
                std::string CurNodeName = std::string(CurNode->Name);
                if(AnimatedNodeNames.find(CurNodeName) != AnimatedNodeNames.end()){
                    RootAnimatedNodeName = CurNodeName;
                    break;
                }
            }
        }
        else{
            RootAnimatedNodeName = RootMotionNodeName;
            
            b32 RootNodeNameExist = AnimatedNodeNames.find(RootAnimatedNodeName) != AnimatedNodeNames.end();
            Assert(RootNodeNameExist);
        }
        
        tool_animation_info NewAnimation;
        
        NewAnimation.Name = std::string(AssimpAnim->mName.C_Str());
        NewAnimation.Duration = AssimpAnim->mDuration;
        NewAnimation.TicksPerSecond = AssimpAnim->mTicksPerSecond;
        NewAnimation.NodesCheckSum = Result.NodesCheckSum;
        NewAnimation.UsesRootMotion = UsesRootMotion;
        
        tool_node_animation* RootMotionNodeAnim = &NewAnimation.RootMotionNodeAnim;
        
        for(int NodeAnimIndex = 0;
            NodeAnimIndex < AssimpAnim->mNumChannels;
            NodeAnimIndex++)
        {
            aiNodeAnim* AssimpNodeAnim = AssimpAnim->mChannels[NodeAnimIndex];
            
            std::string NodeName = std::string(AssimpNodeAnim->mNodeName.C_Str());
            
            b32 IsRootMotionNode = (NodeName == RootAnimatedNodeName);
            
            tool_node_animation NewNodeAnim = {};
            
            // NOTE(Dima): Setting name
            NewNodeAnim.NodeName = NodeName;
            NewNodeAnim.IsRootMotion = false;
            
            // NOTE(Dima): Finding node index in a mapping
            auto FindIterator = Result.NodeNameToNodeIndex.find(NewNodeAnim.NodeName);
            ASSERT(FindIterator != Result.NodeNameToNodeIndex.end());
            NewNodeAnim.NodeIndex = FindIterator->second;
            
            if(IsRootMotionNode)
            {
                RootMotionNodeAnim->IsRootMotion = true;
                RootMotionNodeAnim->NodeIndex = NewNodeAnim.NodeIndex;
                RootMotionNodeAnim->NodeName = NodeName;
            }
            
            // NOTE(Dima): Loading position keys
            for(int KeyIndex = 0; 
                KeyIndex < AssimpNodeAnim->mNumPositionKeys; 
                KeyIndex++)
            {
                aiVectorKey* Key = &AssimpNodeAnim->mPositionKeys[KeyIndex];
                
                float Time = Key->mTime;
                v3 Value = Assimp2JoyVector3(Key->mValue);
                
                if(UsesRootMotion && IsRootMotionNode){
                    v3 Extracted = {};
                    
                    if(ExtractRootMotionY){
                        Extracted.x = Value.x;
                        Extracted.y = 0.0f;
                        Extracted.z = Value.z;
                        
                        Value.x = 0.0f;
                        Value.z = 0.0f;
                    }
                    else if(ExtractRootMotionZ){
                        Extracted.x = Value.x;
                        Extracted.y = Value.y;
                        Extracted.z = 0.0f;
                        
                        Value.x = 0.0f;
                        Value.y = 0.0f;
                    }
                    
                    RootMotionNodeAnim->PositionValues.push_back(Extracted);
                    RootMotionNodeAnim->PositionTimes.push_back(Time);
                }
                
                NewNodeAnim.PositionValues.push_back(Value);
                NewNodeAnim.PositionTimes.push_back(Time);
            }
            
            // NOTE(Dima): Loading rotation keys
            for(int KeyIndex = 0; 
                KeyIndex < AssimpNodeAnim->mNumRotationKeys; 
                KeyIndex++)
            {
                aiQuatKey* Key = &AssimpNodeAnim->mRotationKeys[KeyIndex];
                
                float Time = Key->mTime;
                quat Value = Assimp2JoyQuat(Key->mValue);
                
                // TODO(Dima): Extract rotation nodes animations
                if(ExtractRootMotionY){
                    
                }
                else if(ExtractRootMotionZ){
                    
                }
                
                NewNodeAnim.RotationValues.push_back(Value);
                NewNodeAnim.RotationTimes.push_back(Time);
            }
            
            // NOTE(Dima): Loading scaling keys
            for(int KeyIndex = 0; 
                KeyIndex < AssimpNodeAnim->mNumScalingKeys; 
                KeyIndex++)
            {
                aiVectorKey* Key = &AssimpNodeAnim->mScalingKeys[KeyIndex];
                
                NewNodeAnim.ScalingValues.push_back(Assimp2JoyVector3(Key->mValue));
                NewNodeAnim.ScalingTimes.push_back(Key->mTime);
            }
            
            // NOTE(Dima): Pushing node animation to vector
            NewAnimation.NodeAnimations.push_back(NewNodeAnim);
        }
        
        // NOTE(Dima): Pushing animation to vector
        Result.Animations.push_back(NewAnimation);
    }
    
    if(!IsOnlyAnim){
        // NOTE(Dima): Loading meshes
        for (int MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++) {
            aiMesh* AssimpMesh = scene->mMeshes[MeshIndex];
            
            int MaterialIndex = AssimpMesh->mMaterialIndex;
            
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
            if(Result.HasSkeleton && AssimpMesh->mNumBones){
                Weights.insert(Weights.begin(), AssimpMesh->mNumVertices, {});
                
                // NOTE(Dima): Iterating through bones and getting vertex weights
                for(int AssimpBoneIndex = 0;
                    AssimpBoneIndex < AssimpMesh->mNumBones;
                    AssimpBoneIndex++)
                {
                    
                    aiBone* AssimpBone = AssimpMesh->mBones[AssimpBoneIndex];
                    
                    std::string BoneName(AssimpBone->mName.C_Str());
                    
                    int BoneIndex = BoneNameToBoneIndex[BoneName];
                    
                    for(int VertexWeightIndex = 0;
                        VertexWeightIndex < AssimpBone->mNumWeights;
                        VertexWeightIndex++)
                    {
                        aiVertexWeight* AssimpWeight = &AssimpBone->mWeights[VertexWeightIndex];
                        
                        vertex_weights* Target = &Weights[AssimpWeight->mVertexId];
                        
                        vertex_weight NewWeight;
                        
                        NewWeight.BoneID = BoneIndex;
                        NewWeight.Weight = AssimpWeight->mWeight;
                        
                        Target->Weights.push_back(NewWeight);
                    }
                }
            }
            
            // NOTE(Dima): Generating and pushing mesh if needed
            loaded_mesh_slot MeshSlot = {};
            
            MeshSlot.MeshLoaded = 0;
            MeshSlot.Mesh = MakeMesh(
                                     Vertices,
                                     TexCoords,
                                     Normals,
                                     Tangents,
                                     Indices,
                                     Weights,
                                     JoyShouldCalculateNormals,
                                     JoyShouldCalculateTangents);
            MeshSlot.Mesh.MaterialIndex = MaterialIndex;
            
            Result.Meshes.push_back(MeshSlot);
        }
    }
    
    return(Result);
}

INTERNAL_FUNCTION tool_model_info LoadedToToolModelInfo(loaded_model* Model){
    tool_model_info Result = {};
    
    Result.MeshCount = Model->Meshes.size();
    Result.MaterialCount = Model->Materials.size();
    Result.AnimationCount = Model->Animations.size();
    
    Result.MeshIDs = std::vector<u32>();
    Result.MaterialIDs = std::vector<u32>();
    Result.AnimationIDs = std::vector<u32>();
    
    Result.NodesCheckSum = Model->NodesCheckSum;
    
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

INTERNAL_FUNCTION void StoreAnimationsToGroupID(asset_system* System,
                                                load_model_source* Source,
                                                loaded_model* Model,
                                                u32 AssetGroupID)
{
    Model->ToolModelInfo = LoadedToToolModelInfo(Model);
    tool_model_info* ToolModel = &Model->ToolModelInfo;
    
    // NOTE(Dima): Storing all node animations except root node anim
    BeginAsset(System, AssetEntry_Type_NodeAnim);
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
    
    // NOTE(Dima): Storing root motion node animations and saving IDs
    for(int AnimIndex = 0;
        AnimIndex < Model->Animations.size();
        AnimIndex++)
    {
        tool_animation_info* Animation = &Model->Animations[AnimIndex];
        
        if(Animation->UsesRootMotion){
            
            tool_node_animation* NodeAnim = &Animation->RootMotionNodeAnim;
            
            added_asset Added = AddNodeAnimationAsset(System, NodeAnim);
            Animation->RootMotionNodeAnimID = Added.ID;
        }
        else{
            Animation->RootMotionNodeAnimID = 0xFFFFFFFF;
        }
    }
    EndAsset(System);
    
    // NOTE(Dima): Storing animation assets
    BeginAsset(System, AssetGroupID);
    for(int AnimIndex = 0;
        AnimIndex < Model->Animations.size();
        AnimIndex++)
    {
        tool_animation_info* Animation = &Model->Animations[AnimIndex];
        
        CopyStringsSafe(Animation->StoreName, 
                        sizeof(Animation->StoreName),
                        (char*)Animation->Name.c_str());
        
        added_asset Added = AddAnimationClipAsset(System, Animation);
        if(Source->AnimationImport){
            AddTagHubToAsset(System, &Source->TagHub);
        }
        ToolModel->AnimationIDs.push_back(Added.ID);
    }
    ASSERT(ToolModel->AnimationCount == ToolModel->AnimationIDs.size());
    EndAsset(System);
}

INTERNAL_FUNCTION void StoreAnimAsset(asset_system* System,
                                      load_model_source* Source)
{
    loaded_model* Model = &Source->LoadedModel;
    u32 AssetGroupID = Source->AssetGroup;
    
    // NOTE(Dima): Storing animations
    StoreAnimationsToGroupID(System, Source, 
                             Model, AssetGroupID);
}

INTERNAL_FUNCTION void StoreModelAsset(asset_system* System,
                                       load_model_source* Source,
                                       model_loading_context* Context)
{
    loaded_model* Model = &Source->LoadedModel;
    u32 AssetGroupID = Source->AssetGroup;
    
    Model->ToolModelInfo = LoadedToToolModelInfo(Model);
    tool_model_info* ToolModel = &Model->ToolModelInfo;
    
    // NOTE(Dima): Storing in-model embeded animations
    StoreAnimationsToGroupID(System, Source, Model, 
                             AssetEntry_Type_AnimationClip);
    
    // NOTE(Dima): Storing embeded textures
    BeginAsset(System, AssetEntry_Type_Bitmap);
    for(int EmbedIndex = 0;
        EmbedIndex < Model->EmbededTextures.size();
        EmbedIndex++)
    {
        loaded_mat_texture* MatTex = &Model->EmbededTextures[EmbedIndex];
        
        added_asset Added = AddBitmapAssetManual(System, &MatTex->Bmp);
        MatTex->StoredBitmapID = Added.ID;
    }
    EndAsset(System);
    
    // NOTE(Dima): Storing meshes
    BeginAsset(System, AssetEntry_Type_Mesh);
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
    
    // NOTE(Dima): Storing skeleton
    BeginAsset(System, AssetEntry_Type_Skeleton);
    if(Model->HasSkeleton)
    {
        added_asset Added = AddSkeletonAsset(System, &Model->Skeleton);
        ToolModel->SkeletonID = Added.ID;;
    }
    else{
        ToolModel->SkeletonID = 0;
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
        
        for(int ChannelIndex = 0;
            ChannelIndex < MaterialChannel_Count;
            ChannelIndex++)
        {
            // NOTE(Dima): Getting needed values
            int TextureIndex = Material->TextureIndices[ChannelIndex];
            u32 TextureID = 0;
            
            // NOTE(Dima): Adding bitmap arrays if needed
            u32 AddedArrayID = 0;
            
            if(TextureIndex != -1)
            {
                // NOTE(Dima): Getting first bitmap ID for bitmap array
                // NOTE(Dima): A lot of lookups
                loaded_mat_texture* CorrespondingTexture = 0;
                mat_texture_source* TexSource = &Material->TextureSourceArray[TextureIndex];
                if(TexSource->IsEmbeded){
                    CorrespondingTexture = &Model->EmbededTextures[TexSource->EmbedIndex];
                }
                else{
                    CorrespondingTexture = &Context->PathToTextureMap[TexSource->Path];
                }
                
                TextureID = CorrespondingTexture->StoredBitmapID;
            }
            
            // NOTE(Dima): Setting corresponding array id in material info
            ToolMatInfo->MaterialTextureIDs[ChannelIndex] = TextureID;
            ToolMatInfo->PackedColors[ChannelIndex] = Material->ColorsPacked[ChannelIndex];
        }
        
        BeginAsset(System, AssetEntry_Type_Material);
        added_asset AddedMaterial = AddMaterialAsset(System, ToolMatInfo);
        EndAsset(System);
        
        ToolModel->MaterialIDs.push_back(AddedMaterial.ID);
    }
    
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
    
    // NOTE(Dima): Storing actual model
    BeginAsset(System, AssetGroupID);
    AddModelAsset(System, ToolModel);
    AddTagHubToAsset(System, &Source->TagHub);
    EndAsset(System);
}

INTERNAL_FUNCTION void StoreLoadingContext(asset_system* System, 
                                           model_loading_context* LoadingCtx)
{
    // NOTE(Dima): DirectoryStack should be empty
    Assert(LoadingCtx->DirStack.empty());
    
    // NOTE(Dima): Loading models
    for(int SourceIndex = 0; 
        SourceIndex < LoadingCtx->ModelSources.size();
        SourceIndex++)
    {
        load_model_source* Source = &LoadingCtx->ModelSources[SourceIndex];
        
        Source->LoadedModel = LoadModelByASSIMP((char*)Source->Path.c_str(),
                                                Source->Flags,
                                                LoadingCtx,
                                                Source->RootMotionNodeName);
    }
    
    // NOTE(Dima): Storing bitmaps
    BeginAsset(System, AssetEntry_Type_Bitmap);
    for(auto& it: LoadingCtx->PathToTextureMap){
        added_asset AddedBitmap = AddBitmapAssetManual(System, &it.second.Bmp);
        it.second.StoredBitmapID = AddedBitmap.ID;
    }
    EndAsset(System);
    
    // NOTE(Dima): Storing models or animations
    for(int SourceIndex = 0; 
        SourceIndex < LoadingCtx->ModelSources.size();
        SourceIndex++)
    {
        load_model_source* Source = &LoadingCtx->ModelSources[SourceIndex];
        
        if(Source->AnimationImport){
            StoreAnimAsset(System, Source);
        }
        else{
            StoreModelAsset(System, Source, LoadingCtx);
        }
    }
}
