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

INTERNAL_FUNCTION Asset_Atlas InitAtlas(Memory_Region* region, int Dim){
    Asset_Atlas atlas = {};
    
    mi LargeAtlasMemNeeded = Dim * Dim * 4;
    void* LargeAtlasMem = PushSomeMem(region, LargeAtlasMemNeeded, 16);
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

void InitAssets(Assets* assets, Memory_Region* Region){
    assets->Region = Region;
    
    // NOTE(Dima): Large atlas initialization
    assets->MainLargeAtlas = InitAtlas(Region, 1024);
    
    // NOTE(Dima): images
    Loaded_Strings bmpStrs = LoadStringListFromFile("../Data/Images/ToLoadImages.txt");
    assets->fadeoutBmps = (Bmp_Info*)platform.MemAlloc(sizeof(Bmp_Info) * bmpStrs.Count);
    assets->fadeoutBmpsCount = bmpStrs.Count;
    for(int i = 0; i < bmpStrs.Count; i++){
        char tmpBuf[256];
        stbsp_sprintf(tmpBuf, "../Data/Images/%s", bmpStrs.Strings[i]);
        
        assets->fadeoutBmps[i] = LoadBMP(tmpBuf);
    }
    FreeStringList(&bmpStrs);
    
    // NOTE(Dima): Sunsets
    assets->sunset = LoadBMP("../Data/Images/sunset.jpg");
    assets->sunsetOrange = LoadBMP("../Data/Images/sunset_orange.jpg");
    assets->sunsetField = LoadBMP("../Data/Images/sunset_field.jpg");
    assets->roadClouds = LoadBMP("../Data/Images/road.jpg");
    assets->sunsetMountains = LoadBMP("../Data/Images/sunset_monts.jpg");
    assets->sunsetPurple = LoadBMP("../Data/Images/sunset_purple.jpg");
    assets->sunrise = LoadBMP("../Data/Images/sunrise.jpg");
    assets->mountainsFuji = LoadBMP("../Data/Images/mountains_fuji.jpg");
    
    // NOTE(Dima): Icons
    assets->CheckboxMark = LoadBMP("../Data/Icons/checkmark64.png");
    assets->Folder = LoadBMP("../Data/Icons/folder32.png");
    assets->ClosePng = LoadBMP("../Data/Icons/close.png");
    assets->PlayPng = LoadBMP("../Data/Icons/play.png");
    assets->PlusPng = LoadBMP("../Data/Icons/plus.png");
    assets->StopPng = LoadBMP("../Data/Icons/stop.png");
    assets->PowerPng = LoadBMP("../Data/Icons/power.png");
    
    AddBitmapToAtlas(&assets->MainLargeAtlas, &assets->CheckboxMark);
    AddBitmapToAtlas(&assets->MainLargeAtlas, &assets->Folder);
    AddBitmapToAtlas(&assets->MainLargeAtlas, &assets->ClosePng);
    AddBitmapToAtlas(&assets->MainLargeAtlas, &assets->PlayPng);
    AddBitmapToAtlas(&assets->MainLargeAtlas, &assets->PlusPng);
    AddBitmapToAtlas(&assets->MainLargeAtlas, &assets->StopPng);
    AddBitmapToAtlas(&assets->MainLargeAtlas, &assets->PowerPng);
    
    // NOTE(Dima): Meshes
    assets->cube = MakeCube();
    assets->plane = MakePlane();
    assets->sphere = MakeSphere(20, 12);
    assets->cylynder = MakeCylynder(2.0f, 0.5f, 16);
    
    // NOTE(Dima): Fonts
    assets->liberationMono = LoadFont("../Data/Fonts/LiberationMono-Regular.ttf", 18.0f, LoadFont_BakeShadow);
    assets->lilitaOne = LoadFont("../Data/Fonts/LilitaOne.ttf", 20.0f, LoadFont_BakeShadow);
    
#if 1
    assets->inconsolataBold = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 18.0f, LoadFont_BakeBlur);
#else
    assets->inconsolataBold = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 18.0f, 0);
#endif
    
    assets->pfdin = LoadFont("../Data/Fonts/PFDinTextCondPro-Regular.ttf", 18.0f, 0);
    
    AddFontToAtlas(&assets->MainLargeAtlas, &assets->liberationMono);
    AddFontToAtlas(&assets->MainLargeAtlas, &assets->lilitaOne);
    AddFontToAtlas(&assets->MainLargeAtlas, &assets->inconsolataBold);
    AddFontToAtlas(&assets->MainLargeAtlas, &assets->pfdin);
}