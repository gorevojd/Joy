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
                                        Bmp_Info* bmp, 
                                        v2* OutMinUV, 
                                        v2* OutMaxUV)
{
    if(atlas->AtX + bmp->Width > atlas->Dim){
        atlas->AtX = 0;
        atlas->AtY = atlas->AtY + atlas->MaxInRowHeight;
        atlas->MaxInRowHeight = 0;
    }
    
    ASSERT(atlas->AtY + bmp->Height < atlas->Dim);
    
    float OneOverDim = atlas->OneOverDim;
    
#if 1    
    for (int YIndex = 0; YIndex < bmp->Height; YIndex++) {
        
        for (int XIndex = 0; XIndex < bmp->Width; XIndex++) {
            u32* At = (u32*)bmp->Pixels + YIndex * bmp->Width + XIndex;
            u32* To = (u32*)atlas->Bitmap.Pixels + (atlas->AtY + YIndex) * atlas->Dim + atlas->AtX + XIndex;
            
            *To = *At;
        }
    }
#else
    RenderOneBitmapIntoAnother(
        &atlas->Bitmap,
        bmp,
        atlas->AtX,
        atlas->AtY,
        V4(1.0f, 1.0f, 1.0f, 1.0f));
#endif
    
    if(OutMinUV){
        *OutMinUV = V2(
            (float)atlas->AtX * OneOverDim, 
            (float)atlas->AtY * OneOverDim);
    }
    
    if(OutMaxUV){
        *OutMaxUV = V2(
            (float)(atlas->AtX + bmp->Width) * OneOverDim,
            (float)(atlas->AtY + bmp->Height) * OneOverDim);
    }
    
    atlas->AtX += bmp->Width;
    atlas->MaxInRowHeight = Max(atlas->MaxInRowHeight, bmp->Height);
}

INTERNAL_FUNCTION void AddFontToAtlas(Asset_Atlas* atlas, Font_Info* font){
    for(int i = 0; i < font->GlyphCount; i++){
        Glyph_Info* glyph = &font->Glyphs[i];
        
        AddBitmapToAtlas(atlas,
                         &glyph->Bitmap,
                         &glyph->MinUV,
                         &glyph->MaxUV);
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
    assets->fadeoutBmps = (Bmp_Info*)platform.MemAlloc(sizeof(Bmp_Info) * bmpStrs.count);
    assets->fadeoutBmpsCount = bmpStrs.count;
    for(int i = 0; i < bmpStrs.count; i++){
        char tmpBuf[256];
        stbsp_sprintf(tmpBuf, "../Data/Images/%s", bmpStrs.strings[i]);
        
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
    
    // NOTE(Dima): Check mark
    assets->checkboxMark = LoadBMP("../Data/Icons/checkmark512.png");
    
    // NOTE(Dima): Meshes
    assets->cube = MakeCube();
    assets->plane = MakePlane();
    assets->sphere = MakeSphere(20, 12);
    assets->cylynder = MakeCylynder(2.0f, 0.5f, 16);
    
    // NOTE(Dima): Fonts
    assets->liberationMono = LoadFont("../Data/Fonts/LiberationMono-Regular.ttf", 16.0f, LoadFont_BakeShadow);
    assets->lilitaOne = LoadFont("../Data/Fonts/LilitaOne.ttf", 20.0f, LoadFont_BakeShadow);
    
#if 1
    assets->inconsolataBold = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 18.0f, LoadFont_BakeBlur);
#else
    assets->inconsolataBold = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 16.0f, 0);
#endif
    
    assets->pfdin = LoadFont("../Data/Fonts/PFDinTextCondPro-Regular.ttf", 16.0f, 0);
    
    AddFontToAtlas(&assets->MainLargeAtlas, &assets->liberationMono);
    AddFontToAtlas(&assets->MainLargeAtlas, &assets->lilitaOne);
    AddFontToAtlas(&assets->MainLargeAtlas, &assets->inconsolataBold);
    AddFontToAtlas(&assets->MainLargeAtlas, &assets->pfdin);
}