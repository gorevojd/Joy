#include "tool_asset_build.h"

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

INTERNAL_FUNCTION void StoreFontAsset(asset_system* System, 
                                      u32 GameAssetGroup,
                                      tool_font_info* Font, 
                                      tag_hub* TagHub)
{
    // NOTE(Dima): Adding glyphs
    BeginAsset(System, GameAsset_Type_Glyph);
    for (int GlyphIndex = 0;
         GlyphIndex < Font->GlyphCount;
         GlyphIndex++)
    {
        added_asset AddedGlyphAsset = AddGlyphAsset(System, &Font->Glyphs[GlyphIndex]);
        
        Font->GlyphIDs[GlyphIndex] = AddedGlyphAsset.ID;
    }
    EndAsset(System);
    
    BeginAsset(System, GameAssetGroup);
    AddFontAsset(System, Font);
    AddTagHubToAsset(System, TagHub);
    EndAsset(System);
}

INTERNAL_FUNCTION void WriteFonts(){
    asset_system System_ = {};
    asset_system* System = &System_;
    InitAssetFile(System);
    
    tool_font_info LibMono = LoadFont("../Data/Fonts/LiberationMono-Bold.ttf", 16.0f, LoadFont_BakeBlur);
    tool_font_info Lilita = LoadFont("../Data/Fonts/LilitaOne.ttf", 20.0f, LoadFont_BakeShadow);
    tool_font_info Inconsolata = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 16.0f, LoadFont_BakeBlur);
    tool_font_info PFDIN = LoadFont("../Data/Fonts/PFDinTextCondPro-Regular.ttf", 16.0f, LoadFont_BakeBlur);
    tool_font_info MollyJack = LoadFont("../Data/Fonts/MollyJack.otf", 40.0f, LoadFont_BakeBlur);
    
    tag_hub HubRegular = tag_hub::Empty().AddIntTag(AssetTag_FontType, TagFont_Regular);
    tag_hub HubBold = tag_hub::Empty().AddIntTag(AssetTag_FontType, TagFont_Bold);
    
    StoreFontAsset(System, GameAsset_LiberationMono, &LibMono, &HubRegular);
    StoreFontAsset(System, GameAsset_LilitaOne, &Lilita, &HubRegular);
    StoreFontAsset(System, GameAsset_Inconsolata, &Inconsolata, &HubBold);
    StoreFontAsset(System, GameAsset_PFDIN, &PFDIN, &HubRegular);
    StoreFontAsset(System, GameAsset_MollyJackFont, &MollyJack, &HubRegular);
    
    // NOTE(Dima): Writing file
    WriteAssetFile(System, "../Data/Fonts.ja");
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
    AddArrayAsset(System, FirstBitmapID, BitmapCount);
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