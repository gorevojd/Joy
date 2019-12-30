#include "joy_assets.h"
#include "joy_math.h"
#include "joy_asset_util.h"

#include <vector>
#include <string>

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

void InitAssets(Assets* assets){
    
    Loaded_Strings bmpStrs = LoadStringListFromFile("../Data/Images/ToLoadImages.txt");
    assets->fadeoutBmps = (Bmp_Info*)platform.MemAlloc(sizeof(Bmp_Info) * bmpStrs.count);
    assets->fadeoutBmpsCount = bmpStrs.count;
    for(int i = 0; i < bmpStrs.count; i++){
        char tmpBuf[256];
        stbsp_sprintf(tmpBuf, "../Data/Images/%s", bmpStrs.strings[i]);
        
        assets->fadeoutBmps[i] = LoadBMP(tmpBuf);
    }
    FreeStringList(&bmpStrs);
    
    assets->sunset = LoadBMP("../Data/Images/sunset.jpg");
    assets->sunsetOrange = LoadBMP("../Data/Images/sunset_orange.jpg");
    assets->sunsetField = LoadBMP("../Data/Images/sunset_field.jpg");
    assets->roadClouds = LoadBMP("../Data/Images/road.jpg");
    assets->sunsetMountains = LoadBMP("../Data/Images/sunset_monts.jpg");
    assets->sunsetPurple = LoadBMP("../Data/Images/sunset_purple.jpg");
    assets->sunrise = LoadBMP("../Data/Images/sunrise.jpg");
    assets->mountainsFuji = LoadBMP("../Data/Images/mountains_fuji.jpg");
    
    assets->checkboxMark = LoadBMP("../Data/Icons/checkmark512.png");
    
    assets->liberationMono = LoadFont("../Data/Fonts/LiberationMono-Regular.ttf", 16.0f, LoadFont_BakeShadow);
    assets->lilitaOne = LoadFont("../Data/Fonts/LilitaOne.ttf", 20.0f, LoadFont_BakeShadow);
    
#if 1
    assets->inconsolataBold = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 18.0f, LoadFont_BakeBlur);
#else
    assets->inconsolataBold = LoadFont("../Data/Fonts/Inconsolatazi4-Bold.otf", 16.0f, 0);
#endif
    
    assets->pfdin = LoadFont("../Data/Fonts/PFDinTextCondPro-Regular.ttf", 16.0f, 0);
}