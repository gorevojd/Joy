#include "joy_assets.h"
#include "joy_math.h"
#include "joy_asset_util.h"
#include "joy_software_renderer.h"

#include <vector>
#include <string>

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

INTERNAL_FUNCTION void AddBitmapToAtlas(Asset_Atlas* atlas, 
                                        Bmp_Info* bmp)
{
    In_Atlas_Bitmap Result = {};
    
    int Border = 3;
    
    int ActualWidth = bmp->Width + Border * 2;
    int ActualHeight = bmp->Height + Border * 2;
    
    if(atlas->AtX + ActualWidth > atlas->Dim){
        atlas->AtX = 0;
        atlas->AtY = atlas->AtY + atlas->MaxInRowHeight;
        atlas->MaxInRowHeight = 0;
    }
    
    ASSERT(atlas->AtY + ActualHeight  < atlas->Dim);
    
    float OneOverDim = atlas->OneOverDim;
    
    for (int YIndex = 0; YIndex < bmp->Height; YIndex++) {
        
        for (int XIndex = 0; XIndex < bmp->Width; XIndex++) {
            u32* At = (u32*)bmp->Pixels + YIndex * bmp->Width + XIndex;
            u32* To = (u32*)atlas->Bitmap.Pixels + (atlas->AtY + YIndex + Border) * atlas->Dim + atlas->AtX + XIndex + Border;
            
            *To = *At;
        }
    }
    
    bmp->MinUV = V2(
        (float)(atlas->AtX + Border) * OneOverDim, 
        (float)(atlas->AtY + Border) * OneOverDim);
    
    bmp->MaxUV = V2(
        (float)(atlas->AtX + Border + bmp->Width) * OneOverDim,
        (float)(atlas->AtY + Border + bmp->Height) * OneOverDim);
    
    atlas->AtX += ActualWidth;
    atlas->MaxInRowHeight = Max(atlas->MaxInRowHeight, ActualHeight);
}

INTERNAL_FUNCTION void AddFontToAtlas(Asset_Atlas* atlas, Font_Info* font){
    for(int i = 0; i < font->GlyphCount; i++){
        Glyph_Info* glyph = &font->Glyphs[i];
        
        AddBitmapToAtlas(atlas, &glyph->Bitmap);
    }
}

INTERNAL_FUNCTION Asset_Atlas InitAtlas(mem_region* Region, int Dim){
    Asset_Atlas atlas = {};
    
    mi LargeAtlasMemNeeded = Dim * Dim * 4;
    void* LargeAtlasMem = PushSomeMem(Region, LargeAtlasMemNeeded, 16);
    atlas.Bitmap = AllocateBitmapInternal(
        Dim, 
        Dim,
        LargeAtlasMem);
    atlas.Dim = Dim;
    atlas.AtX = 0;
    atlas.AtY = 0;
    atlas.MaxInRowHeight = 0;
    atlas.OneOverDim = 1.0f / (float)Dim;
    
    return(atlas);
}

void InitAssets(assets* Assets){
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // NOTE(Dima): Memory region is already initialized
    // NOTE(Dima): !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    // NOTE(Dima): Large atlas initialization
    Assets->MainLargeAtlas = InitAtlas(Assets->Region, 1024);
    
    // NOTE(Dima): images
    Loaded_Strings bmpStrs = LoadStringListFromFile("../Data/Images/ToLoadImages.txt");
    Assets->fadeoutBmps = (Bmp_Info*)malloc(sizeof(Bmp_Info) * bmpStrs.Count);
    Assets->fadeoutBmpsCount = bmpStrs.Count;
    for(int i = 0; i < bmpStrs.Count; i++){
        char tmpBuf[256];
        stbsp_sprintf(tmpBuf, "../Data/Images/%s", bmpStrs.Strings[i]);
        
        Assets->fadeoutBmps[i] = LoadBMP(tmpBuf);
    }
    FreeStringList(&bmpStrs);
    
    // NOTE(Dima): Sunsets
    Assets->sunset = LoadBMP("../Data/Images/sunset.jpg");
    Assets->sunsetOrange = LoadBMP("../Data/Images/sunset_orange.jpg");
    Assets->sunsetField = LoadBMP("../Data/Images/sunset_field.jpg");
    Assets->roadClouds = LoadBMP("../Data/Images/road.jpg");
    Assets->sunsetMountains = LoadBMP("../Data/Images/sunset_monts.jpg");
    Assets->sunsetPurple = LoadBMP("../Data/Images/sunset_purple.jpg");
    Assets->sunrise = LoadBMP("../Data/Images/sunrise.jpg");
    Assets->mountainsFuji = LoadBMP("../Data/Images/mountains_fuji.jpg");
    
    // NOTE(Dima): Icons
    Assets->CheckboxMark = LoadBMP("../Data/Icons/checkmark64.png");
    Assets->Folder = LoadBMP("../Data/Icons/folder32.png");
    Assets->ClosePng = LoadBMP("../Data/Icons/close.png");
    Assets->PlayPng = LoadBMP("../Data/Icons/play.png");
    Assets->PlusPng = LoadBMP("../Data/Icons/plus.png");
    Assets->StopPng = LoadBMP("../Data/Icons/stop.png");
    Assets->PowerPng = LoadBMP("../Data/Icons/power.png");
    
    AddBitmapToAtlas(&Assets->MainLargeAtlas, &Assets->CheckboxMark);
    AddBitmapToAtlas(&Assets->MainLargeAtlas, &Assets->Folder);
    AddBitmapToAtlas(&Assets->MainLargeAtlas, &Assets->ClosePng);
    AddBitmapToAtlas(&Assets->MainLargeAtlas, &Assets->PlayPng);
    AddBitmapToAtlas(&Assets->MainLargeAtlas, &Assets->PlusPng);
    AddBitmapToAtlas(&Assets->MainLargeAtlas, &Assets->StopPng);
    AddBitmapToAtlas(&Assets->MainLargeAtlas, &Assets->PowerPng);
    
    // NOTE(Dima): Sounds
    Assets->SineTest1 = MakeSineSound256(44100 * 4, 44100);
    Assets->SineTest2 = MakeSineSound(std::vector<int>{256, 128, 430}, 44100 * 4, 44100);
    
    // NOTE(Dima): Meshes
    Assets->Cube = MakeCube();
    Assets->Plane = MakePlane();
    Assets->Sphere = MakeSphere(20, 12);
    Assets->Cylynder = MakeCylynder(2.0f, 0.5f, 16);
    
    // NOTE(Dima): Fonts
    Assets->liberationMono = LoadFont("../Data/Fonts/LiberationMono-Regular.ttf", 18.0f, LoadFont_BakeShadow);
    Assets->lilitaOne = LoadFont("../Data/Fonts/LilitaOne.ttf", 20.0f, LoadFont_BakeShadow);
    
#if 1
    Assets->inconsolataBold = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 18.0f, LoadFont_BakeBlur);
#else
    Assets->inconsolataBold = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 18.0f, 0);
#endif
    Assets->MollyJackFont = LoadFont("../Data/Fonts/MollyJack.otf", 40.0f, LoadFont_BakeBlur);
    
    Assets->pfdin = LoadFont("../Data/Fonts/PFDinTextCondPro-Regular.ttf", 18.0f, LoadFont_BakeBlur);
    
    AddFontToAtlas(&Assets->MainLargeAtlas, &Assets->liberationMono);
    AddFontToAtlas(&Assets->MainLargeAtlas, &Assets->lilitaOne);
    AddFontToAtlas(&Assets->MainLargeAtlas, &Assets->inconsolataBold);
    AddFontToAtlas(&Assets->MainLargeAtlas, &Assets->pfdin);
    AddFontToAtlas(&Assets->MainLargeAtlas, &Assets->MollyJackFont);
}