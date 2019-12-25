#include "joy_assets.h"
#include "joy_math.h"
#include "joy_asset_util.h"

void InitAssets(Assets* assets){
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