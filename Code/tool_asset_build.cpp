#include "tool_asset_build.h"

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

INTERNAL_FUNCTION void WriteFontsChunk(tool_font_info** Fonts,
                                       u32* FontsGroups,
                                       game_asset_tag_hub* FontsTags,
                                       int FontsCount, 
                                       int ChunkIndex)
{
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    // NOTE(Dima): Adding glyphs
    BeginAsset(System, GameAsset_Type_Glyph);
    for (int FontIndex = 0;
         FontIndex < FontsCount;
         FontIndex++)
    {
        tool_font_info* Font = Fonts[FontIndex];
        
        for (int GlyphIndex = 0;
             GlyphIndex < Font->GlyphCount;
             GlyphIndex++)
        {
            added_asset AddedGlyphAsset = AddGlyphAsset(System, &Font->Glyphs[GlyphIndex]);
            
            Font->GlyphIDs[GlyphIndex] = AddedGlyphAsset.ID;
        }
    }
    EndAsset(System);
    
    // NOTE(Dima): Adding fonts
    for (int FontIndex = 0;
         FontIndex < FontsCount;
         FontIndex++)
    {
        tool_font_info* Font = Fonts[FontIndex];
        game_asset_tag_hub* TagHub = FontsTags + FontIndex;
        
        BeginAsset(System, FontsGroups[FontIndex]);
        // NOTE(Dima): Adding font asset
        AddFontAsset(System, Font);
        // NOTE(Dima): Adding font tags
        for(int TagIndex = 0; 
            TagIndex < TagHub->TagCount;
            TagIndex++)
        {
            game_asset_tag* Tag = TagHub->Tags + TagIndex;
            
            switch(TagHub->TagValueTypes[TagIndex]){
                case GameAssetTagValue_Float:{
                    AddFloatTag(System, Tag->Type, Tag->Value_Float);
                }break;
                
                case GameAssetTagValue_Int:{
                    AddIntTag(System, Tag->Type, Tag->Value_Int);
                }break;
                
                case GameAssetTagValue_Empty:{
                    AddEmptyTag(System, Tag->Type);
                }break;
            }
        }
        EndAsset(System);
    }
    
    // NOTE(Dima): Writing file
    char OutFileName[256];
    stbsp_sprintf(OutFileName, "../Data/Fonts%d.ja", ChunkIndex);
    
    WriteAssetFile(System, OutFileName);
}

INTERNAL_FUNCTION void WriteFonts(){
    tool_font_info LibMono = LoadFont("../Data/Fonts/LiberationMono-Regular.ttf", 16.0f, LoadFont_BakeShadow);
    tool_font_info Lilita = LoadFont("../Data/Fonts/LilitaOne.ttf", 20.0f, LoadFont_BakeShadow);
    tool_font_info Inconsolata = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 16.0f, LoadFont_BakeBlur);
    tool_font_info PFDIN = LoadFont("../Data/Fonts/PFDinTextCondPro-Regular.ttf", 16.0f, LoadFont_BakeBlur);
    tool_font_info MollyJack = LoadFont("../Data/Fonts/MollyJack.otf", 40.0f, LoadFont_BakeBlur);
    
    tool_font_info* Fonts[] = {
        &LibMono,
        &Lilita,
        &Inconsolata,
        &PFDIN,
        &MollyJack,
    };
    
    u32 Groups[] = {
        GameAsset_LiberationMono,
        GameAsset_LilitaOne,
        GameAsset_Inconsolata,
        GameAsset_PFDIN,
        GameAsset_MollyJackFont,
    };
    
    game_asset_tag_hub Tags[] = {
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Bold),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
        game_asset_tag_hub::Empty().AddIntTag(AssetTag_FontType, AssetFontTypeTag_Regular),
    };
    
    int FontCount = ArrayCount(Fonts);
    
    int ChunkIndex = 0;
    for(int FontIndex = 0;
        FontIndex < FontCount;
        FontIndex += ASSET_GROUP_REGIONS_COUNT,
        ChunkIndex++)
    {
        tool_font_info** CurFontsChunk = &Fonts[FontIndex];
        u32 * CurChunkGroups = Groups + FontIndex;
        game_asset_tag_hub* CurChunkTagHub = Tags + FontIndex;
        
        int CurChunkSize = Min(FontCount - FontIndex, ASSET_GROUP_REGIONS_COUNT);
        
        if(CurChunkSize){
            WriteFontsChunk(CurFontsChunk, 
                            CurChunkGroups,
                            CurChunkTagHub,
                            CurChunkSize, 
                            ChunkIndex);
        }
    }
}

INTERNAL_FUNCTION void WriteIcons(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    BeginAsset(System, GameAsset_CheckboxMark);
    AddIconAsset(System, "../Data/Icons/checkmark64.png");
    EndAsset(System);
    
    BeginAsset(System, GameAsset_ChamomileIcon);
    AddIconAsset(System, "../Data/Icons/chamomile.png");
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/Icons.ja");
}

INTERNAL_FUNCTION void WriteBitmapArray(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    std::string FolderPath("../Data/Images/");
    std::string ListFileName("ToLoadImages.txt");
    
    std::vector<std::string> BitmapPaths;
    
    // NOTE(Dima): Initializing bitmap paths
    Loaded_Strings BitmapNames = LoadStringListFromFile((char*)(FolderPath + ListFileName).c_str());
    int BitmapCount = BitmapNames.Count;
    for(int i = 0; i < BitmapCount; i++){
        BitmapPaths.push_back(FolderPath + std::string(BitmapNames.Strings[i]));
    }
    FreeStringList(&BitmapNames);
    
    // NOTE(Dima): Adding bitmap assets
    BeginAsset(System, GameAsset_Type_Bitmap);
    u32 FirstBitmapID = 0;
    for(int i = 0; i < BitmapPaths.size(); i++){
        added_asset Asset = AddBitmapAsset(System, (char*)BitmapPaths[i].c_str());
        
        if(i == 0){
            FirstBitmapID = Asset.ID;
        }
    }
    EndAsset(System);
    
    // NOTE(Dima): adding bitmap array
    BeginAsset(System, GameAsset_FadeoutBmps);
    AddBitmapArray(System, FirstBitmapID, BitmapCount);
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/BitmapsArray.ja");
}

INTERNAL_FUNCTION void WriteBitmaps(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    
    
    WriteAssetFile(System, "../Data/Bitmaps.ja");
}

INTERNAL_FUNCTION void WriteSounds(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    tool_sound_info Sine = MakeSineSound256(44100 * 2, 44100);
    
    BeginAsset(System, GameAsset_SineTest);
    AddSoundAssetManual(System, &Sine);
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/Sounds.ja");
}

INTERNAL_FUNCTION void WriteMeshPrimitives(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    tool_mesh_info Cube = MakeCube();
    tool_mesh_info Plane = MakePlane();
    
    tool_mesh_info SphereMeshSuperHig = MakeSphere(160, 80);
    tool_mesh_info SphereMeshHig = MakeSphere(80, 40);
    tool_mesh_info SphereMeshAvg = MakeSphere(40, 20);
    tool_mesh_info SphereMeshLow = MakeSphere(20, 10);
    tool_mesh_info SphereMeshSuperLow = MakeSphere(10, 5);
    
    tool_mesh_info CylMeshSuperLow = MakeCylynder(2.0f, 0.5f, 6);
    tool_mesh_info CylMeshLow = MakeCylynder(2.0f, 0.5f, 12);
    tool_mesh_info CylMeshAvg = MakeCylynder(2.0f, 0.5f, 24);
    tool_mesh_info CylMeshHig = MakeCylynder(2.0f, 0.5f, 48);
    tool_mesh_info CylMeshSuperHig = MakeCylynder(2.0f, 0.5f, 96);
    
    BeginAsset(System, GameAsset_Cube);
    AddMeshAsset(System, &Cube);
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Plane);
    AddMeshAsset(System, &Plane);
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Sphere);
    AddMeshAsset(System, &SphereMeshSuperLow);
    AddFloatTag(System, AssetTag_LOD, 0.0f);
    AddMeshAsset(System, &SphereMeshLow);
    AddFloatTag(System, AssetTag_LOD, 0.25f);
    AddMeshAsset(System, &SphereMeshAvg);
    AddFloatTag(System, AssetTag_LOD, 0.5f);
    AddMeshAsset(System, &SphereMeshHig);
    AddFloatTag(System, AssetTag_LOD, 0.75f);
    AddMeshAsset(System, &SphereMeshSuperHig);
    AddFloatTag(System, AssetTag_LOD, 1.0f);
    EndAsset(System);
    
    BeginAsset(System, GameAsset_Cylynder);
    AddMeshAsset(System, &CylMeshSuperLow);
    AddFloatTag(System, AssetTag_LOD, 0.0f);
    AddMeshAsset(System, &CylMeshLow);
    AddFloatTag(System, AssetTag_LOD, 0.25f);
    AddMeshAsset(System, &CylMeshAvg);
    AddFloatTag(System, AssetTag_LOD, 0.5f);
    AddMeshAsset(System, &CylMeshHig);
    AddFloatTag(System, AssetTag_LOD, 0.75f);
    AddMeshAsset(System, &CylMeshSuperHig);
    AddFloatTag(System, AssetTag_LOD, 1.0f);
    EndAsset(System);
    
    WriteAssetFile(System, "../Data/MeshPrimitives.ja");
}

int main() {
    WriteBitmaps();
    WriteIcons();
    WriteMeshPrimitives();
    WriteFonts();
    WriteBitmapArray();
    WriteSounds();
    
    system("pause");
    return(0);
}