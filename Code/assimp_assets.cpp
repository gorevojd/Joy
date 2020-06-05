#include "tool_asset_build_assimp.cpp"

inline void BeginRootMotionNodeName(model_loading_context* Ctx, char* Name){
    Assert(!Ctx->RootMotionNodeNameBeginned);
    
    Ctx->RootMotionNodeNameBeginned = true;
    Ctx->RootMotionNodeName = std::string(Name);
}

inline void EndRootMotionNodeName(model_loading_context* Ctx){
    Assert(Ctx->RootMotionNodeNameBeginned);
    
    Ctx->RootMotionNodeNameBeginned = false;
    Ctx->RootMotionNodeName = "";
}

inline void PushDirectory(model_loading_context* Ctx, char* DirPath){
    // NOTE(Dima): DirectoryStack should be empty
    Assert(Ctx->DirStack.empty());
    
    Ctx->DirStack.push(std::string(DirPath));
}

inline void PopDirectory(model_loading_context* Ctx){
    // NOTE(Dima): DirectoryStack should be empty
    Assert(!Ctx->DirStack.empty());
    Assert(Ctx->DirStack.size() == 1);
    
    Ctx->DirStack.pop();
}

inline std::string GetAssetPathForLoadingContext(model_loading_context* Ctx, char* InitPath){
    std::string Result = "";
    
    if(!Ctx->DirStack.empty()){
        Result += Ctx->DirStack.top();
        
        if(Result[Result.length() - 1] != '/'){
            Result += "/";
        }
    }
    
    Result += std::string(InitPath);
    
    return(Result);
}

inline void AddModelSource(model_loading_context* Ctx, 
                           char* Path, 
                           u32 AssetGroup,
                           u32 Flags)
{
    std::string NewPath = GetAssetPathForLoadingContext(Ctx, Path);
    
    load_model_source Source = ModelSource(NewPath, AssetGroup, 
                                           Flags, Ctx->TagHub,
                                           Ctx->RootMotionNodeName);
    
    Ctx->ModelSources.push_back(Source);
}

inline void AddAnimSource(model_loading_context* Ctx, 
                          char* Path, 
                          u32 AssetGroup,
                          u32 Flags,
                          b32 IsLooped)
{
    std::string NewPath = GetAssetPathForLoadingContext(Ctx, Path);
    
    if(IsLooped){
        Flags |= Load_AnimationWillBeLooped;
    }
    
    load_model_source Source = ModelSource(NewPath, AssetGroup, 
                                           Flags | Load_ImportOnlyAnimation, 
                                           Ctx->TagHub,
                                           Ctx->RootMotionNodeName);
    
    Ctx->ModelSources.push_back(Source);
}

inline void BeginCharacter(model_loading_context* Ctx, u32 CharacterTagValue){
    Assert(!Ctx->CharacterBeginned);
    Ctx->CharacterBeginned = true;
    
    Ctx->TagHub.AddIntTag(AssetTag_Character, CharacterTagValue);
}

inline void EndCharacter(model_loading_context* Ctx){
    Assert(Ctx->CharacterBeginned);
    Ctx->CharacterBeginned = false;
    
    Ctx->TagHub.PopTag();
}

inline void PushIntTag(model_loading_context* Ctx, u32 AssetTag, u32 TagValue){
    Ctx->TagHub.AddIntTag(AssetTag, TagValue);
}

inline void PopTag(model_loading_context* Ctx){
    Ctx->TagHub.PopTag();
}

INTERNAL_FUNCTION void WriteMeshes1(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    model_loading_context LoadCtx = {};
    model_loading_context* Ctx = &LoadCtx;
    
    u32 DefaultFlags = 
        Load_GenerateNormals |
        Load_GenerateTangents;
    
    AddModelSource(Ctx, "../Data/Models/Animations/Male_Casual.fbx",
                   GameAsset_Man,
                   DefaultFlags);
    
    // NOTE(Dima): Storing loading context
    StoreLoadingContext(System, Ctx);
    
    WriteAssetFile(System, "../Data/AssimpMeshes1.ja");
}


INTERNAL_FUNCTION void AddCharacterToWrite(model_loading_context* Ctx, 
                                           char* DataFolder,
                                           u32 TagCharacterValue,
                                           char* ModelName)
{
    u32 DefaultFlags = 
        Load_GenerateNormals |
        Load_GenerateTangents;
    
    BeginCharacter(Ctx, TagCharacterValue);
    PushDirectory(Ctx, DataFolder);
    
    AddModelSource(Ctx, ModelName,
                   GameAsset_Model_Character, DefaultFlags);
    
    AddAnimSource(Ctx, "animations/Failure.fbx", 
                  GameAsset_Anim_Failure, DefaultFlags, 
                  LOAD_ANIM_NOT_LOOPING);
    
    AddAnimSource(Ctx, "animations/Fall.fbx",
                  GameAsset_Anim_Fall, DefaultFlags, 
                  LOAD_ANIM_LOOPING);
    
    PushIntTag(Ctx, AssetTag_IdleAnim, TagIdleAnim_Idle0);
    AddAnimSource(Ctx, "animations/Idle.fbx",
                  GameAsset_Anim_Idle, DefaultFlags, 
                  LOAD_ANIM_LOOPING);
    PopTag(Ctx);
    
    PushIntTag(Ctx, AssetTag_IdleAnim, TagIdleAnim_Idle1);
    AddAnimSource(Ctx, "animations/Idle_2.fbx",
                  GameAsset_Anim_Idle, DefaultFlags, 
                  LOAD_ANIM_LOOPING);
    PopTag(Ctx);
    
    AddAnimSource(Ctx, "animations/Jump_Up.fbx",
                  GameAsset_Anim_JumpUp, DefaultFlags, 
                  LOAD_ANIM_LOOPING);
    
    AddAnimSource(Ctx, "animations/Land.fbx",
                  GameAsset_Anim_Land, DefaultFlags, 
                  LOAD_ANIM_NOT_LOOPING);
    
    AddAnimSource(Ctx, "animations/Roll.fbx",
                  GameAsset_Anim_Roll, DefaultFlags | Load_ExtractRootMotionZ, 
                  LOAD_ANIM_NOT_LOOPING);
    
    AddAnimSource(Ctx, "animations/Run.fbx",
                  GameAsset_Anim_Run, DefaultFlags | Load_ExtractRootMotionZ, 
                  LOAD_ANIM_LOOPING);
    
    AddAnimSource(Ctx, "animations/Sleep.fbx",
                  GameAsset_Anim_Sleep, DefaultFlags, 
                  LOAD_ANIM_LOOPING);
    
    AddAnimSource(Ctx, "animations/Success.fbx",
                  GameAsset_Anim_Success, DefaultFlags, 
                  LOAD_ANIM_NOT_LOOPING);
    
    AddAnimSource(Ctx, "animations/Talk.fbx",
                  GameAsset_Anim_Talk, DefaultFlags, 
                  LOAD_ANIM_LOOPING);
    
    AddAnimSource(Ctx, "animations/Walk.fbx",
                  GameAsset_Anim_Walk, DefaultFlags | Load_ExtractRootMotionZ, 
                  LOAD_ANIM_LOOPING);
    PopDirectory(Ctx);
    EndCharacter(Ctx);
}

INTERNAL_FUNCTION void AddCaterpillar(model_loading_context* Ctx)
{
    u32 DefaultFlags = 
        Load_GenerateNormals |
        Load_GenerateTangents;
    
    char* NameFBX = "Caterpillar.fbx";
    char* Directory = "../Data/Models/Insects/Caterpillar";
    
    BeginCharacter(Ctx, TagCharacter_Caterpillar);
    PushDirectory(Ctx, Directory);
    
    // NOTE(Dima): Common
    AddModelSource(Ctx, NameFBX,
                   GameAsset_Model_Character, DefaultFlags);
    
    AddAnimSource(Ctx, "animations/Fall.fbx",
                  GameAsset_Anim_Fall, DefaultFlags, LOAD_ANIM_LOOPING);
    
    PushIntTag(Ctx, AssetTag_IdleAnim, TagIdleAnim_Idle0);
    AddAnimSource(Ctx, "animations/Idle.fbx",
                  GameAsset_Anim_Idle, DefaultFlags, 
                  LOAD_ANIM_LOOPING);
    PopTag(Ctx);
    
    AddAnimSource(Ctx, "animations/Jump_Up.fbx",
                  GameAsset_Anim_JumpUp, DefaultFlags, 
                  LOAD_ANIM_LOOPING);
    
    AddAnimSource(Ctx, "animations/Land.fbx",
                  GameAsset_Anim_Land, DefaultFlags, 
                  LOAD_ANIM_NOT_LOOPING);
    
    AddAnimSource(Ctx, "animations/Roll.fbx",
                  GameAsset_Anim_Roll, 
                  DefaultFlags | Load_ExtractRootMotionZ, 
                  LOAD_ANIM_NOT_LOOPING);
    
    AddAnimSource(Ctx, "animations/Run.fbx",
                  GameAsset_Anim_Run, 
                  DefaultFlags | Load_ExtractRootMotionZ, 
                  LOAD_ANIM_LOOPING);
    
    AddAnimSource(Ctx, "animations/Die.fbx",
                  GameAsset_Anim_Die, DefaultFlags, 
                  LOAD_ANIM_NOT_LOOPING);
    
    PopDirectory(Ctx);
    EndCharacter(Ctx);
}

INTERNAL_FUNCTION void WriteForestAnimals(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    model_loading_context LoadCtx = {};
    model_loading_context* Ctx = &LoadCtx;
    
    AddCharacterToWrite(Ctx, "../Data/Models/ForestAnimals/Deer",
                        TagCharacter_Deer,
                        "Deer.fbx");
    
    AddCharacterToWrite(Ctx, "../Data/Models/ForestAnimals/Rabbit",
                        TagCharacter_Rabbit,
                        "Rabbit.fbx");
    
    AddCharacterToWrite(Ctx, "../Data/Models/ForestAnimals/Fox",
                        TagCharacter_Fox,
                        "Fox.fbx");
    
    AddCharacterToWrite(Ctx, "../Data/Models/ForestAnimals/Moose",
                        TagCharacter_Moose,
                        "Moose.fbx");
    
    AddCharacterToWrite(Ctx, "../Data/Models/ForestAnimals/Bear",
                        TagCharacter_Bear,
                        "Bear.fbx");
    
    AddCharacterToWrite(Ctx, "../Data/Models/ForestAnimals/Coyote",
                        TagCharacter_Coyote,
                        "Coyote.fbx");
    
    // NOTE(Dima): Storing loading context
    StoreLoadingContext(System, Ctx);
    
    WriteAssetFile(System, "../Data/ForestAnimals.ja");
}


INTERNAL_FUNCTION void WriteFarmAnimals(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    model_loading_context LoadCtx = {};
    model_loading_context* Ctx = &LoadCtx;
    
    AddCharacterToWrite(Ctx, "../Data/Models/FarmAnimals/Bull",
                        TagCharacter_Bull,
                        "Bull.fbx");
    
    AddCharacterToWrite(Ctx, "../Data/Models/FarmAnimals/Chicken",
                        TagCharacter_Chicken,
                        "Chicken.fbx");
    
    AddCharacterToWrite(Ctx, "../Data/Models/FarmAnimals/Cow",
                        TagCharacter_Cow,
                        "Cow.fbx");
    
    AddCharacterToWrite(Ctx, "../Data/Models/FarmAnimals/Horse",
                        TagCharacter_Horse,
                        "Horse.fbx");
    
    AddCharacterToWrite(Ctx, "../Data/Models/FarmAnimals/Pig",
                        TagCharacter_Pig,
                        "Pig.fbx");
    
#if 0    
    AddCharacterToWrite(Ctx, "../Data/Models/FarmAnimals/Sheep",
                        TagCharacter_Sheep,
                        "Sheep.fbx");
#endif
    
    // NOTE(Dima): Storing loading context
    StoreLoadingContext(System, Ctx);
    
    WriteAssetFile(System, "../Data/FarmAnimals.ja");
}

INTERNAL_FUNCTION void WriteInsects(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    model_loading_context LoadCtx = {};
    model_loading_context* Ctx = &LoadCtx;
    
    AddCaterpillar(Ctx);
    
#if 0    
    AddInsectCharacter(Ctx, "../Data/Models/Insects/Hornet",
                       TagCharacter_Hornet,
                       "Hornet.fbx");
    
    AddInsectCharacter(Ctx, "../Data/Models/Insects/LadyBug",
                       TagCharacter_LadyBug,
                       "Ladybug.fbx");
    
    AddInsectCharacter(Ctx, "../Data/Models/Insects/Mantis",
                       TagCharacter_Mantis,
                       "Mantis.fbx");
    
    AddInsectCharacter(Ctx, "../Data/Models/Insects/Moth",
                       TagCharacter_Moth,
                       "Moth.fbx");
    
    AddInsectCharacter(Ctx, "../Data/Models/Insects/Rhinoceros Beetle",
                       TagCharacter_Rhinoceros,
                       "Rhinoceros_Beetle.FBX");
#endif
    
    // NOTE(Dima): Storing loading context
    StoreLoadingContext(System, Ctx);
    
    WriteAssetFile(System, "../Data/Insects.ja");
}

int main(int ArgsCount, char** Args){
    
    WriteInsects();
    WriteForestAnimals();
    WriteFarmAnimals();
    WriteMeshes1();
    
    system("pause");
    return(0);
}