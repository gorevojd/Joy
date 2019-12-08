#include "joy_assets.h"
#include "joy_math.h"
#include "joy_asset_util.h"

void InitAssets(assets* Assets){
    Assets->Sunset = LoadBMP("../Data/Images/sunset.jpg");
    Assets->SunsetOrange = LoadBMP("../Data/Images/sunset_orange.jpg");
    Assets->SunsetField = LoadBMP("../Data/Images/sunset_field.jpg");
    Assets->RoadClouds = LoadBMP("../Data/Images/road.jpg");
    Assets->SunsetMountains = LoadBMP("../Data/Images/sunset_monts.jpg");
    Assets->SunsetPurple = LoadBMP("../Data/Images/sunset_purple.jpg");
    Assets->Sunrise = LoadBMP("../Data/Images/sunrise.jpg");
    Assets->MountainsFuji = LoadBMP("../Data/Images/mountains_fuji.jpg");
    
    Assets->CheckboxMark = LoadBMP("../Data/Icons/checkmark512.png");
    
    Assets->LiberationMono = LoadFont("../Data/Fonts/LiberationMono-Regular.ttf", 16.0f, LoadFont_BakeShadow);
}