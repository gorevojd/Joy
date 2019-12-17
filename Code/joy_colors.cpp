#include "joy_colors.h"
#include "joy_strings.h"

inline color_slot CreateColorSlot(color_state* ColorState, v4 Color, char* Name) {
	color_slot Res = {};
    
	Res.Color = Color;
	Res.Name = PushString(ColorState->ColorsMem, Name);
	CopyStrings(Res.Name, Name);
    
	char* At = Res.Name;
	while (*At) {
        
		if (*At >= 'a' && *At <= 'z') {
			*At = *At - 'a' + 'A';
		}
        
		At++;
	}
    
	Res.ColorU32 = PackRGBA(Res.Color);
    
	return(Res);
}

#define JOY_SET_COLOR(colorname, v4color) ColorState->ColorTable[Color_##colorname] = CreateColorSlot(ColorState, v4color, #colorname)
static void InitDefaultColors(color_state* ColorState){
	//NOTE(Dima): Initialization of the color table
	JOY_SET_COLOR(Black,  V4(0.0f, 0.0f, 0.0f, 1.0f));
    JOY_SET_COLOR(White,  V4(1.0f, 1.0f, 1.0f, 1.0f));
	JOY_SET_COLOR(Red,  V4(1.0f, 0.0f, 0.0f, 1.0f));
	JOY_SET_COLOR(Green,  V4(0.0f, 1.0f, 0.0f, 1.0f));
	JOY_SET_COLOR(Blue,  V4(0.0f, 0.0f, 1.0f, 1.0f));
	JOY_SET_COLOR(Yellow,  V4(1.0f, 1.0f, 0.0f, 1.0f));
	JOY_SET_COLOR(Magenta,  V4(1.0f, 0.0f, 1.0f, 1.0f));
	JOY_SET_COLOR(Cyan,  V4(0.0f, 1.0f, 1.0f, 1.0f));
	JOY_SET_COLOR(PrettyBlue,  V4(0.0f, 0.5f, 1.0f, 1.0f));
	JOY_SET_COLOR(Purple,  ColorFrom255(85, 26, 139));
	JOY_SET_COLOR(Orange,  ColorFromHex("#ffa500"));
	JOY_SET_COLOR(Amber,  ColorFromHex("#FF7E00"));
	JOY_SET_COLOR(Brown,  ColorFromHex("#993300"));
	JOY_SET_COLOR(Burlywood,  ColorFromHex("#deb887"));
	JOY_SET_COLOR(DarkGoldenrod,  ColorFromHex("#b8860b"));
	JOY_SET_COLOR(OliveDrab,  ColorFromHex("#6b8e23"));
    
    JOY_SET_COLOR(Thistle,  ColorFromHex("#e6e6fa"));
    JOY_SET_COLOR(Plum,  ColorFromHex("#DDA0DD"));
    JOY_SET_COLOR(Violet,  ColorFromHex("#EE82EE"));
    JOY_SET_COLOR(Orchid,  ColorFromHex("#DA70D6"));
    JOY_SET_COLOR(Fuchsia,  ColorFromHex("#FF00FF"));
    JOY_SET_COLOR(MediumOrchid,  ColorFromHex("#BA55D3"));
    JOY_SET_COLOR(MediumPurple,  ColorFromHex("#9370DB"));
    JOY_SET_COLOR(BlueViolet,  ColorFromHex("#8a2be2"));
    JOY_SET_COLOR(DarkViolet,  ColorFromHex("#9400d3"));
    JOY_SET_COLOR(DarkOrchid,  ColorFromHex("#9932CC"));
    JOY_SET_COLOR(DarkMagenta,  ColorFromHex("#8B008B"));
    JOY_SET_COLOR(Indigo,  ColorFromHex("#4b0082"));
    JOY_SET_COLOR(SlateBlue,  ColorFromHex("#6a5acd"));
    JOY_SET_COLOR(DarkSlateBlue,  ColorFromHex("#483d8b"));
}

#define JOY_SET_EXTCOLOR(colorname, v4color) ColorState->ColorTable[ColorExt_##colorname] = CreateColorSlot(ColorState, v4color, #colorname)
static void InitExtColors(color_state* ColorState) {
    JOY_SET_EXTCOLOR(AliceBlue, ColorFromHex("#f0f8ff"));
	JOY_SET_EXTCOLOR(AntiqueWhite, ColorFromHex("#faebd7"));
	JOY_SET_EXTCOLOR(AntiqueWhite1, ColorFromHex("#ffefdb"));
	JOY_SET_EXTCOLOR(AntiqueWhite2, ColorFromHex("#eedfcc"));
	JOY_SET_EXTCOLOR(AntiqueWhite3, ColorFromHex("#cdc0b0"));
	JOY_SET_EXTCOLOR(AntiqueWhite4, ColorFromHex("#8b8378"));
	JOY_SET_EXTCOLOR(aquamarine1, ColorFromHex("#7fffd4"));
	JOY_SET_EXTCOLOR(aquamarine2, ColorFromHex("#76eec6"));
	JOY_SET_EXTCOLOR(aquamarine4, ColorFromHex("#458b74"));
	JOY_SET_EXTCOLOR(azure1, ColorFromHex("#f0ffff"));
	JOY_SET_EXTCOLOR(azure2, ColorFromHex("#e0eeee"));
	JOY_SET_EXTCOLOR(azure3, ColorFromHex("#c1cdcd"));
	JOY_SET_EXTCOLOR(azure4, ColorFromHex("#838b8b"));
	JOY_SET_EXTCOLOR(beige, ColorFromHex("#f5f5dc"));
	JOY_SET_EXTCOLOR(bisque1, ColorFromHex("#ffe4c4"));
	JOY_SET_EXTCOLOR(bisque2, ColorFromHex("#eed5b7"));
	JOY_SET_EXTCOLOR(bisque3, ColorFromHex("#cdb79e"));
	JOY_SET_EXTCOLOR(bisque4, ColorFromHex("#8b7d6b"));
	JOY_SET_EXTCOLOR(black, ColorFromHex("#000000"));
	JOY_SET_EXTCOLOR(BlanchedAlmond, ColorFromHex("#ffebcd"));
	JOY_SET_EXTCOLOR(blue1, ColorFromHex("#0000ff"));
	JOY_SET_EXTCOLOR(blue2, ColorFromHex("#0000ee"));
	JOY_SET_EXTCOLOR(blue4, ColorFromHex("#00008b"));
	JOY_SET_EXTCOLOR(BlueViolet, ColorFromHex("#8a2be2"));
	JOY_SET_EXTCOLOR(brown, ColorFromHex("#a52a2a"));
	JOY_SET_EXTCOLOR(brown1, ColorFromHex("#ff4040"));
	JOY_SET_EXTCOLOR(brown2, ColorFromHex("#ee3b3b"));
	JOY_SET_EXTCOLOR(brown3, ColorFromHex("#cd3333"));
	JOY_SET_EXTCOLOR(brown4, ColorFromHex("#8b2323"));
	JOY_SET_EXTCOLOR(burlywood, ColorFromHex("#deb887"));
	JOY_SET_EXTCOLOR(burlywood1, ColorFromHex("#ffd39b"));
	JOY_SET_EXTCOLOR(burlywood2, ColorFromHex("#eec591"));
	JOY_SET_EXTCOLOR(burlywood3, ColorFromHex("#cdaa7d"));
	JOY_SET_EXTCOLOR(burlywood4, ColorFromHex("#8b7355"));
	JOY_SET_EXTCOLOR(CadetBlue, ColorFromHex("#5f9ea0"));
	JOY_SET_EXTCOLOR(CadetBlue1, ColorFromHex("#98f5ff"));
	JOY_SET_EXTCOLOR(CadetBlue2, ColorFromHex("#8ee5ee"));
	JOY_SET_EXTCOLOR(CadetBlue3, ColorFromHex("#7ac5cd"));
	JOY_SET_EXTCOLOR(CadetBlue4, ColorFromHex("#53868b"));
	JOY_SET_EXTCOLOR(chartreuse1, ColorFromHex("#7fff00"));
	JOY_SET_EXTCOLOR(chartreuse2, ColorFromHex("#76ee00"));
	JOY_SET_EXTCOLOR(chartreuse3, ColorFromHex("#66cd00"));
	JOY_SET_EXTCOLOR(chartreuse4, ColorFromHex("#458b00"));
	JOY_SET_EXTCOLOR(chocolate, ColorFromHex("#d2691e"));
	JOY_SET_EXTCOLOR(chocolate1, ColorFromHex("#ff7f24"));
	JOY_SET_EXTCOLOR(chocolate2, ColorFromHex("#ee7621"));
	JOY_SET_EXTCOLOR(chocolate3, ColorFromHex("#cd661d"));
	JOY_SET_EXTCOLOR(coral, ColorFromHex("#ff7f50"));
	JOY_SET_EXTCOLOR(coral1, ColorFromHex("#ff7256"));
	JOY_SET_EXTCOLOR(coral2, ColorFromHex("#ee6a50"));
	JOY_SET_EXTCOLOR(coral3, ColorFromHex("#cd5b45"));
	JOY_SET_EXTCOLOR(coral4, ColorFromHex("#8b3e2f"));
	JOY_SET_EXTCOLOR(CornflowerBlue, ColorFromHex("#6495ed"));
	JOY_SET_EXTCOLOR(cornsilk1, ColorFromHex("#fff8dc"));
	JOY_SET_EXTCOLOR(cornsilk2, ColorFromHex("#eee8cd"));
	JOY_SET_EXTCOLOR(cornsilk3, ColorFromHex("#cdc8b1"));
	JOY_SET_EXTCOLOR(cornsilk4, ColorFromHex("#8b8878"));
	JOY_SET_EXTCOLOR(cyan1, ColorFromHex("#00ffff"));
	JOY_SET_EXTCOLOR(cyan2, ColorFromHex("#00eeee"));
	JOY_SET_EXTCOLOR(cyan3, ColorFromHex("#00cdcd"));
	JOY_SET_EXTCOLOR(cyan4, ColorFromHex("#008b8b"));
	JOY_SET_EXTCOLOR(DarkGoldenrod, ColorFromHex("#b8860b"));
	JOY_SET_EXTCOLOR(DarkGoldenrod1, ColorFromHex("#ffb90f"));
	JOY_SET_EXTCOLOR(DarkGoldenrod2, ColorFromHex("#eead0e"));
	JOY_SET_EXTCOLOR(DarkGoldenrod3, ColorFromHex("#cd950c"));
	JOY_SET_EXTCOLOR(DarkGoldenrod4, ColorFromHex("#8b6508"));
	JOY_SET_EXTCOLOR(DarkGreen, ColorFromHex("#006400"));
	JOY_SET_EXTCOLOR(DarkKhaki, ColorFromHex("#bdb76b"));
	JOY_SET_EXTCOLOR(DarkOliveGreen, ColorFromHex("#556b2f"));
	JOY_SET_EXTCOLOR(DarkOliveGreen1, ColorFromHex("#caff70"));
	JOY_SET_EXTCOLOR(DarkOliveGreen2, ColorFromHex("#bcee68"));
	JOY_SET_EXTCOLOR(DarkOliveGreen3, ColorFromHex("#a2cd5a"));
	JOY_SET_EXTCOLOR(DarkOliveGreen4, ColorFromHex("#6e8b3d"));
	JOY_SET_EXTCOLOR(DarkOrange, ColorFromHex("#ff8c00"));
	JOY_SET_EXTCOLOR(DarkOrange1, ColorFromHex("#ff7f00"));
	JOY_SET_EXTCOLOR(DarkOrange2, ColorFromHex("#ee7600"));
	JOY_SET_EXTCOLOR(DarkOrange3, ColorFromHex("#cd6600"));
	JOY_SET_EXTCOLOR(DarkOrange4, ColorFromHex("#8b4500"));
	JOY_SET_EXTCOLOR(DarkOrchid, ColorFromHex("#9932cc"));
	JOY_SET_EXTCOLOR(DarkOrchid1, ColorFromHex("#bf3eff"));
	JOY_SET_EXTCOLOR(DarkOrchid2, ColorFromHex("#b23aee"));
	JOY_SET_EXTCOLOR(DarkOrchid3, ColorFromHex("#9a32cd"));
	JOY_SET_EXTCOLOR(DarkOrchid4, ColorFromHex("#68228b"));
	JOY_SET_EXTCOLOR(DarkSalmon, ColorFromHex("#e9967a"));
	JOY_SET_EXTCOLOR(DarkSeaGreen, ColorFromHex("#8fbc8f"));
	JOY_SET_EXTCOLOR(DarkSeaGreen1, ColorFromHex("#c1ffc1"));
	JOY_SET_EXTCOLOR(DarkSeaGreen2, ColorFromHex("#b4eeb4"));
	JOY_SET_EXTCOLOR(DarkSeaGreen3, ColorFromHex("#9bcd9b"));
	JOY_SET_EXTCOLOR(DarkSeaGreen4, ColorFromHex("#698b69"));
	JOY_SET_EXTCOLOR(DarkSlateBlue, ColorFromHex("#483d8b"));
	JOY_SET_EXTCOLOR(DarkSlateGray, ColorFromHex("#2f4f4f"));
	JOY_SET_EXTCOLOR(DarkSlateGray1, ColorFromHex("#97ffff"));
	JOY_SET_EXTCOLOR(DarkSlateGray2, ColorFromHex("#8deeee"));
	JOY_SET_EXTCOLOR(DarkSlateGray3, ColorFromHex("#79cdcd"));
	JOY_SET_EXTCOLOR(DarkSlateGray4, ColorFromHex("#528b8b"));
	JOY_SET_EXTCOLOR(DarkTurquoise, ColorFromHex("#00ced1"));
	JOY_SET_EXTCOLOR(DarkViolet, ColorFromHex("#9400d3"));
	JOY_SET_EXTCOLOR(DeepPink1, ColorFromHex("#ff1493"));
	JOY_SET_EXTCOLOR(DeepPink2, ColorFromHex("#ee1289"));
	JOY_SET_EXTCOLOR(DeepPink3, ColorFromHex("#cd1076"));
	JOY_SET_EXTCOLOR(DeepPink4, ColorFromHex("#8b0a50"));
	JOY_SET_EXTCOLOR(DeepSkyBlue1, ColorFromHex("#00bfff"));
	JOY_SET_EXTCOLOR(DeepSkyBlue2, ColorFromHex("#00b2ee"));
	JOY_SET_EXTCOLOR(DeepSkyBlue3, ColorFromHex("#009acd"));
	JOY_SET_EXTCOLOR(DeepSkyBlue4, ColorFromHex("#00688b"));
	JOY_SET_EXTCOLOR(DimGray, ColorFromHex("#696969"));
	JOY_SET_EXTCOLOR(DodgerBlue1, ColorFromHex("#1e90ff"));
	JOY_SET_EXTCOLOR(DodgerBlue2, ColorFromHex("#1c86ee"));
	JOY_SET_EXTCOLOR(DodgerBlue3, ColorFromHex("#1874cd"));
	JOY_SET_EXTCOLOR(DodgerBlue4, ColorFromHex("#104e8b"));
	JOY_SET_EXTCOLOR(firebrick, ColorFromHex("#b22222"));
	JOY_SET_EXTCOLOR(firebrick1, ColorFromHex("#ff3030"));
	JOY_SET_EXTCOLOR(firebrick2, ColorFromHex("#ee2c2c"));
	JOY_SET_EXTCOLOR(firebrick3, ColorFromHex("#cd2626"));
	JOY_SET_EXTCOLOR(firebrick4, ColorFromHex("#8b1a1a"));
	JOY_SET_EXTCOLOR(FloralWhite, ColorFromHex("#fffaf0"));
	JOY_SET_EXTCOLOR(ForestGreen, ColorFromHex("#228b22"));
	JOY_SET_EXTCOLOR(gainsboro, ColorFromHex("#dcdcdc"));
	JOY_SET_EXTCOLOR(GhostWhite, ColorFromHex("#f8f8ff"));
	JOY_SET_EXTCOLOR(gold1, ColorFromHex("#ffd700"));
	JOY_SET_EXTCOLOR(gold2, ColorFromHex("#eec900"));
	JOY_SET_EXTCOLOR(gold3, ColorFromHex("#cdad00"));
	JOY_SET_EXTCOLOR(gold4, ColorFromHex("#8b7500"));
	JOY_SET_EXTCOLOR(goldenrod, ColorFromHex("#daa520"));
	JOY_SET_EXTCOLOR(goldenrod1, ColorFromHex("#ffc125"));
	JOY_SET_EXTCOLOR(goldenrod2, ColorFromHex("#eeb422"));
	JOY_SET_EXTCOLOR(goldenrod3, ColorFromHex("#cd9b1d"));
	JOY_SET_EXTCOLOR(goldenrod4, ColorFromHex("#8b6914"));
	JOY_SET_EXTCOLOR(gray, ColorFromHex("#bebebe"));
	JOY_SET_EXTCOLOR(gray1, ColorFromHex("#030303"));
	JOY_SET_EXTCOLOR(gray2, ColorFromHex("#050505"));
	JOY_SET_EXTCOLOR(gray3, ColorFromHex("#080808"));
	JOY_SET_EXTCOLOR(gray4, ColorFromHex("#0a0a0a"));
	JOY_SET_EXTCOLOR(gray5, ColorFromHex("#0d0d0d"));
	JOY_SET_EXTCOLOR(gray6, ColorFromHex("#0f0f0f"));
	JOY_SET_EXTCOLOR(gray7, ColorFromHex("#121212"));
	JOY_SET_EXTCOLOR(gray8, ColorFromHex("#141414"));
	JOY_SET_EXTCOLOR(gray9, ColorFromHex("#171717"));
	JOY_SET_EXTCOLOR(gray10, ColorFromHex("#1a1a1a"));
	JOY_SET_EXTCOLOR(gray11, ColorFromHex("#1c1c1c"));
	JOY_SET_EXTCOLOR(gray12, ColorFromHex("#1f1f1f"));
	JOY_SET_EXTCOLOR(gray13, ColorFromHex("#212121"));
	JOY_SET_EXTCOLOR(gray14, ColorFromHex("#242424"));
	JOY_SET_EXTCOLOR(gray15, ColorFromHex("#262626"));
	JOY_SET_EXTCOLOR(gray16, ColorFromHex("#292929"));
	JOY_SET_EXTCOLOR(gray17, ColorFromHex("#2b2b2b"));
	JOY_SET_EXTCOLOR(gray18, ColorFromHex("#2e2e2e"));
	JOY_SET_EXTCOLOR(gray19, ColorFromHex("#303030"));
	JOY_SET_EXTCOLOR(gray20, ColorFromHex("#333333"));
	JOY_SET_EXTCOLOR(gray21, ColorFromHex("#363636"));
	JOY_SET_EXTCOLOR(gray22, ColorFromHex("#383838"));
	JOY_SET_EXTCOLOR(gray23, ColorFromHex("#3b3b3b"));
	JOY_SET_EXTCOLOR(gray24, ColorFromHex("#3d3d3d"));
	JOY_SET_EXTCOLOR(gray25, ColorFromHex("#404040"));
	JOY_SET_EXTCOLOR(gray26, ColorFromHex("#424242"));
	JOY_SET_EXTCOLOR(gray27, ColorFromHex("#454545"));
	JOY_SET_EXTCOLOR(gray28, ColorFromHex("#474747"));
	JOY_SET_EXTCOLOR(gray29, ColorFromHex("#4a4a4a"));
	JOY_SET_EXTCOLOR(gray30, ColorFromHex("#4d4d4d"));
	JOY_SET_EXTCOLOR(gray31, ColorFromHex("#4f4f4f"));
	JOY_SET_EXTCOLOR(gray32, ColorFromHex("#525252"));
	JOY_SET_EXTCOLOR(gray33, ColorFromHex("#545454"));
	JOY_SET_EXTCOLOR(gray34, ColorFromHex("#575757"));
	JOY_SET_EXTCOLOR(gray35, ColorFromHex("#595959"));
	JOY_SET_EXTCOLOR(gray36, ColorFromHex("#5c5c5c"));
	JOY_SET_EXTCOLOR(gray37, ColorFromHex("#5e5e5e"));
	JOY_SET_EXTCOLOR(gray38, ColorFromHex("#616161"));
	JOY_SET_EXTCOLOR(gray39, ColorFromHex("#636363"));
	JOY_SET_EXTCOLOR(gray40, ColorFromHex("#666666"));
	JOY_SET_EXTCOLOR(gray41, ColorFromHex("#696969"));
	JOY_SET_EXTCOLOR(gray42, ColorFromHex("#6b6b6b"));
	JOY_SET_EXTCOLOR(gray43, ColorFromHex("#6e6e6e"));
	JOY_SET_EXTCOLOR(gray44, ColorFromHex("#707070"));
	JOY_SET_EXTCOLOR(gray45, ColorFromHex("#737373"));
	JOY_SET_EXTCOLOR(gray46, ColorFromHex("#757575"));
	JOY_SET_EXTCOLOR(gray47, ColorFromHex("#787878"));
	JOY_SET_EXTCOLOR(gray48, ColorFromHex("#7a7a7a"));
	JOY_SET_EXTCOLOR(gray49, ColorFromHex("#7d7d7d"));
	JOY_SET_EXTCOLOR(gray50, ColorFromHex("#7f7f7f"));
	JOY_SET_EXTCOLOR(gray51, ColorFromHex("#828282"));
	JOY_SET_EXTCOLOR(gray52, ColorFromHex("#858585"));
	JOY_SET_EXTCOLOR(gray53, ColorFromHex("#878787"));
	JOY_SET_EXTCOLOR(gray54, ColorFromHex("#8a8a8a"));
	JOY_SET_EXTCOLOR(gray55, ColorFromHex("#8c8c8c"));
	JOY_SET_EXTCOLOR(gray56, ColorFromHex("#8f8f8f"));
	JOY_SET_EXTCOLOR(gray57, ColorFromHex("#919191"));
	JOY_SET_EXTCOLOR(gray58, ColorFromHex("#949494"));
	JOY_SET_EXTCOLOR(gray59, ColorFromHex("#969696"));
	JOY_SET_EXTCOLOR(gray60, ColorFromHex("#999999"));
	JOY_SET_EXTCOLOR(gray61, ColorFromHex("#9c9c9c"));
	JOY_SET_EXTCOLOR(gray62, ColorFromHex("#9e9e9e"));
	JOY_SET_EXTCOLOR(gray63, ColorFromHex("#a1a1a1"));
	JOY_SET_EXTCOLOR(gray64, ColorFromHex("#a3a3a3"));
	JOY_SET_EXTCOLOR(gray65, ColorFromHex("#a6a6a6"));
	JOY_SET_EXTCOLOR(gray66, ColorFromHex("#a8a8a8"));
	JOY_SET_EXTCOLOR(gray67, ColorFromHex("#ababab"));
	JOY_SET_EXTCOLOR(gray68, ColorFromHex("#adadad"));
	JOY_SET_EXTCOLOR(gray69, ColorFromHex("#b0b0b0"));
	JOY_SET_EXTCOLOR(gray70, ColorFromHex("#b3b3b3"));
	JOY_SET_EXTCOLOR(gray71, ColorFromHex("#b5b5b5"));
	JOY_SET_EXTCOLOR(gray72, ColorFromHex("#b8b8b8"));
	JOY_SET_EXTCOLOR(gray73, ColorFromHex("#bababa"));
	JOY_SET_EXTCOLOR(gray74, ColorFromHex("#bdbdbd"));
	JOY_SET_EXTCOLOR(gray75, ColorFromHex("#bfbfbf"));
	JOY_SET_EXTCOLOR(gray76, ColorFromHex("#c2c2c2"));
	JOY_SET_EXTCOLOR(gray77, ColorFromHex("#c4c4c4"));
	JOY_SET_EXTCOLOR(gray78, ColorFromHex("#c7c7c7"));
	JOY_SET_EXTCOLOR(gray79, ColorFromHex("#c9c9c9"));
	JOY_SET_EXTCOLOR(gray80, ColorFromHex("#cccccc"));
	JOY_SET_EXTCOLOR(gray81, ColorFromHex("#cfcfcf"));
	JOY_SET_EXTCOLOR(gray82, ColorFromHex("#d1d1d1"));
	JOY_SET_EXTCOLOR(gray83, ColorFromHex("#d4d4d4"));
	JOY_SET_EXTCOLOR(gray84, ColorFromHex("#d6d6d6"));
	JOY_SET_EXTCOLOR(gray85, ColorFromHex("#d9d9d9"));
	JOY_SET_EXTCOLOR(gray86, ColorFromHex("#dbdbdb"));
	JOY_SET_EXTCOLOR(gray87, ColorFromHex("#dedede"));
	JOY_SET_EXTCOLOR(gray88, ColorFromHex("#e0e0e0"));
	JOY_SET_EXTCOLOR(gray89, ColorFromHex("#e3e3e3"));
	JOY_SET_EXTCOLOR(gray90, ColorFromHex("#e5e5e5"));
	JOY_SET_EXTCOLOR(gray91, ColorFromHex("#e8e8e8"));
	JOY_SET_EXTCOLOR(gray92, ColorFromHex("#ebebeb"));
	JOY_SET_EXTCOLOR(gray93, ColorFromHex("#ededed"));
	JOY_SET_EXTCOLOR(gray94, ColorFromHex("#f0f0f0"));
	JOY_SET_EXTCOLOR(gray95, ColorFromHex("#f2f2f2"));
	JOY_SET_EXTCOLOR(gray97, ColorFromHex("#f7f7f7"));
	JOY_SET_EXTCOLOR(gray98, ColorFromHex("#fafafa"));
	JOY_SET_EXTCOLOR(gray99, ColorFromHex("#fcfcfc"));
	JOY_SET_EXTCOLOR(green1, ColorFromHex("#00ff00"));
	JOY_SET_EXTCOLOR(green2, ColorFromHex("#00ee00"));
	JOY_SET_EXTCOLOR(green3, ColorFromHex("#00cd00"));
	JOY_SET_EXTCOLOR(green4, ColorFromHex("#008b00"));
	JOY_SET_EXTCOLOR(GreenYellow, ColorFromHex("#adff2f"));
	JOY_SET_EXTCOLOR(honeydew1, ColorFromHex("#f0fff0"));
	JOY_SET_EXTCOLOR(honeydew2, ColorFromHex("#e0eee0"));
	JOY_SET_EXTCOLOR(honeydew3, ColorFromHex("#c1cdc1"));
	JOY_SET_EXTCOLOR(honeydew4, ColorFromHex("#838b83"));
	JOY_SET_EXTCOLOR(HotPink, ColorFromHex("#ff69b4"));
	JOY_SET_EXTCOLOR(HotPink1, ColorFromHex("#ff6eb4"));
	JOY_SET_EXTCOLOR(HotPink2, ColorFromHex("#ee6aa7"));
	JOY_SET_EXTCOLOR(HotPink3, ColorFromHex("#cd6090"));
	JOY_SET_EXTCOLOR(HotPink4, ColorFromHex("#8b3a62"));
	JOY_SET_EXTCOLOR(IndianRed, ColorFromHex("#cd5c5c"));
	JOY_SET_EXTCOLOR(IndianRed1, ColorFromHex("#ff6a6a"));
	JOY_SET_EXTCOLOR(IndianRed2, ColorFromHex("#ee6363"));
	JOY_SET_EXTCOLOR(IndianRed3, ColorFromHex("#cd5555"));
	JOY_SET_EXTCOLOR(IndianRed4, ColorFromHex("#8b3a3a"));
	JOY_SET_EXTCOLOR(ivory1, ColorFromHex("#fffff0"));
	JOY_SET_EXTCOLOR(ivory2, ColorFromHex("#eeeee0"));
	JOY_SET_EXTCOLOR(ivory3, ColorFromHex("#cdcdc1"));
	JOY_SET_EXTCOLOR(ivory4, ColorFromHex("#8b8b83"));
	JOY_SET_EXTCOLOR(khaki, ColorFromHex("#f0e68c"));
	JOY_SET_EXTCOLOR(khaki1, ColorFromHex("#fff68f"));
	JOY_SET_EXTCOLOR(khaki2, ColorFromHex("#eee685"));
	JOY_SET_EXTCOLOR(khaki3, ColorFromHex("#cdc673"));
	JOY_SET_EXTCOLOR(khaki4, ColorFromHex("#8b864e"));
	JOY_SET_EXTCOLOR(lavender, ColorFromHex("#e6e6fa"));
	JOY_SET_EXTCOLOR(LavenderBlush1, ColorFromHex("#fff0f5"));
	JOY_SET_EXTCOLOR(LavenderBlush2, ColorFromHex("#eee0e5"));
	JOY_SET_EXTCOLOR(LavenderBlush3, ColorFromHex("#cdc1c5"));
	JOY_SET_EXTCOLOR(LavenderBlush4, ColorFromHex("#8b8386"));
	JOY_SET_EXTCOLOR(LawnGreen, ColorFromHex("#7cfc00"));
	JOY_SET_EXTCOLOR(LemonChiffon1, ColorFromHex("#fffacd"));
	JOY_SET_EXTCOLOR(LemonChiffon2, ColorFromHex("#eee9bf"));
	JOY_SET_EXTCOLOR(LemonChiffon3, ColorFromHex("#cdc9a5"));
	JOY_SET_EXTCOLOR(LemonChiffon4, ColorFromHex("#8b8970"));
	JOY_SET_EXTCOLOR(light, ColorFromHex("#eedd82"));
	JOY_SET_EXTCOLOR(LightBlue, ColorFromHex("#add8e6"));
	JOY_SET_EXTCOLOR(LightBlue1, ColorFromHex("#bfefff"));
	JOY_SET_EXTCOLOR(LightBlue2, ColorFromHex("#b2dfee"));
	JOY_SET_EXTCOLOR(LightBlue3, ColorFromHex("#9ac0cd"));
	JOY_SET_EXTCOLOR(LightBlue4, ColorFromHex("#68838b"));
	JOY_SET_EXTCOLOR(LightCoral, ColorFromHex("#f08080"));
	JOY_SET_EXTCOLOR(LightCyan1, ColorFromHex("#e0ffff"));
	JOY_SET_EXTCOLOR(LightCyan2, ColorFromHex("#d1eeee"));
	JOY_SET_EXTCOLOR(LightCyan3, ColorFromHex("#b4cdcd"));
	JOY_SET_EXTCOLOR(LightCyan4, ColorFromHex("#7a8b8b"));
	JOY_SET_EXTCOLOR(LightGoldenrod1, ColorFromHex("#ffec8b"));
	JOY_SET_EXTCOLOR(LightGoldenrod2, ColorFromHex("#eedc82"));
	JOY_SET_EXTCOLOR(LightGoldenrod3, ColorFromHex("#cdbe70"));
	JOY_SET_EXTCOLOR(LightGoldenrod4, ColorFromHex("#8b814c"));
	JOY_SET_EXTCOLOR(LightGoldenrodYellow, ColorFromHex("#fafad2"));
	JOY_SET_EXTCOLOR(LightGray, ColorFromHex("#d3d3d3"));
	JOY_SET_EXTCOLOR(LightPink, ColorFromHex("#ffb6c1"));
	JOY_SET_EXTCOLOR(LightPink1, ColorFromHex("#ffaeb9"));
	JOY_SET_EXTCOLOR(LightPink2, ColorFromHex("#eea2ad"));
	JOY_SET_EXTCOLOR(LightPink3, ColorFromHex("#cd8c95"));
	JOY_SET_EXTCOLOR(LightPink4, ColorFromHex("#8b5f65"));
	JOY_SET_EXTCOLOR(LightSalmon1, ColorFromHex("#ffa07a"));
	JOY_SET_EXTCOLOR(LightSalmon2, ColorFromHex("#ee9572"));
	JOY_SET_EXTCOLOR(LightSalmon3, ColorFromHex("#cd8162"));
	JOY_SET_EXTCOLOR(LightSalmon4, ColorFromHex("#8b5742"));
	JOY_SET_EXTCOLOR(LightSeaGreen, ColorFromHex("#20b2aa"));
	JOY_SET_EXTCOLOR(LightSkyBlue, ColorFromHex("#87cefa"));
	JOY_SET_EXTCOLOR(LightSkyBlue1, ColorFromHex("#b0e2ff"));
	JOY_SET_EXTCOLOR(LightSkyBlue2, ColorFromHex("#a4d3ee"));
	JOY_SET_EXTCOLOR(LightSkyBlue3, ColorFromHex("#8db6cd"));
	JOY_SET_EXTCOLOR(LightSkyBlue4, ColorFromHex("#607b8b"));
	JOY_SET_EXTCOLOR(LightSlateBlue, ColorFromHex("#8470ff"));
	JOY_SET_EXTCOLOR(LightSlateGray, ColorFromHex("#778899"));
	JOY_SET_EXTCOLOR(LightSteelBlue, ColorFromHex("#b0c4de"));
	JOY_SET_EXTCOLOR(LightSteelBlue1, ColorFromHex("#cae1ff"));
	JOY_SET_EXTCOLOR(LightSteelBlue2, ColorFromHex("#bcd2ee"));
	JOY_SET_EXTCOLOR(LightSteelBlue3, ColorFromHex("#a2b5cd"));
	JOY_SET_EXTCOLOR(LightSteelBlue4, ColorFromHex("#6e7b8b"));
	JOY_SET_EXTCOLOR(LightYellow1, ColorFromHex("#ffffe0"));
	JOY_SET_EXTCOLOR(LightYellow2, ColorFromHex("#eeeed1"));
	JOY_SET_EXTCOLOR(LightYellow3, ColorFromHex("#cdcdb4"));
	JOY_SET_EXTCOLOR(LightYellow4, ColorFromHex("#8b8b7a"));
	JOY_SET_EXTCOLOR(LimeGreen, ColorFromHex("#32cd32"));
	JOY_SET_EXTCOLOR(linen, ColorFromHex("#faf0e6"));
	JOY_SET_EXTCOLOR(magenta, ColorFromHex("#ff00ff"));
	JOY_SET_EXTCOLOR(magenta2, ColorFromHex("#ee00ee"));
	JOY_SET_EXTCOLOR(magenta3, ColorFromHex("#cd00cd"));
	JOY_SET_EXTCOLOR(magenta4, ColorFromHex("#8b008b"));
	JOY_SET_EXTCOLOR(maroon, ColorFromHex("#b03060"));
	JOY_SET_EXTCOLOR(maroon1, ColorFromHex("#ff34b3"));
	JOY_SET_EXTCOLOR(maroon2, ColorFromHex("#ee30a7"));
	JOY_SET_EXTCOLOR(maroon3, ColorFromHex("#cd2990"));
	JOY_SET_EXTCOLOR(maroon4, ColorFromHex("#8b1c62"));
	JOY_SET_EXTCOLOR(medium, ColorFromHex("#66cdaa"));
	JOY_SET_EXTCOLOR(MediumAquamarine, ColorFromHex("#66cdaa"));
	JOY_SET_EXTCOLOR(MediumBlue, ColorFromHex("#0000cd"));
	JOY_SET_EXTCOLOR(MediumOrchid, ColorFromHex("#ba55d3"));
	JOY_SET_EXTCOLOR(MediumOrchid1, ColorFromHex("#e066ff"));
	JOY_SET_EXTCOLOR(MediumOrchid2, ColorFromHex("#d15fee"));
	JOY_SET_EXTCOLOR(MediumOrchid3, ColorFromHex("#b452cd"));
	JOY_SET_EXTCOLOR(MediumOrchid4, ColorFromHex("#7a378b"));
	JOY_SET_EXTCOLOR(MediumPurple, ColorFromHex("#9370db"));
	JOY_SET_EXTCOLOR(MediumPurple1, ColorFromHex("#ab82ff"));
	JOY_SET_EXTCOLOR(MediumPurple2, ColorFromHex("#9f79ee"));
	JOY_SET_EXTCOLOR(MediumPurple3, ColorFromHex("#8968cd"));
	JOY_SET_EXTCOLOR(MediumPurple4, ColorFromHex("#5d478b"));
	JOY_SET_EXTCOLOR(MediumSeaGreen, ColorFromHex("#3cb371"));
	JOY_SET_EXTCOLOR(MediumSlateBlue, ColorFromHex("#7b68ee"));
	JOY_SET_EXTCOLOR(MediumSpringGreen, ColorFromHex("#00fa9a"));
	JOY_SET_EXTCOLOR(MediumTurquoise, ColorFromHex("#48d1cc"));
	JOY_SET_EXTCOLOR(MediumVioletRed, ColorFromHex("#c71585"));
	JOY_SET_EXTCOLOR(MidnightBlue, ColorFromHex("#191970"));
	JOY_SET_EXTCOLOR(MintCream, ColorFromHex("#f5fffa"));
	JOY_SET_EXTCOLOR(MistyRose1, ColorFromHex("#ffe4e1"));
	JOY_SET_EXTCOLOR(MistyRose2, ColorFromHex("#eed5d2"));
	JOY_SET_EXTCOLOR(MistyRose3, ColorFromHex("#cdb7b5"));
	JOY_SET_EXTCOLOR(MistyRose4, ColorFromHex("#8b7d7b"));
	JOY_SET_EXTCOLOR(moccasin, ColorFromHex("#ffe4b5"));
	JOY_SET_EXTCOLOR(NavajoWhite1, ColorFromHex("#ffdead"));
	JOY_SET_EXTCOLOR(NavajoWhite2, ColorFromHex("#eecfa1"));
	JOY_SET_EXTCOLOR(NavajoWhite3, ColorFromHex("#cdb38b"));
	JOY_SET_EXTCOLOR(NavajoWhite4, ColorFromHex("#8b795e"));
	JOY_SET_EXTCOLOR(NavyBlue, ColorFromHex("#000080"));
	JOY_SET_EXTCOLOR(OldLace, ColorFromHex("#fdf5e6"));
	JOY_SET_EXTCOLOR(OliveDrab, ColorFromHex("#6b8e23"));
	JOY_SET_EXTCOLOR(OliveDrab1, ColorFromHex("#c0ff3e"));
	JOY_SET_EXTCOLOR(OliveDrab2, ColorFromHex("#b3ee3a"));
	JOY_SET_EXTCOLOR(OliveDrab4, ColorFromHex("#698b22"));
	JOY_SET_EXTCOLOR(orange1, ColorFromHex("#ffa500"));
	JOY_SET_EXTCOLOR(orange2, ColorFromHex("#ee9a00"));
	JOY_SET_EXTCOLOR(orange3, ColorFromHex("#cd8500"));
	JOY_SET_EXTCOLOR(orange4, ColorFromHex("#8b5a00"));
	JOY_SET_EXTCOLOR(OrangeRed1, ColorFromHex("#ff4500"));
	JOY_SET_EXTCOLOR(OrangeRed2, ColorFromHex("#ee4000"));
	JOY_SET_EXTCOLOR(OrangeRed3, ColorFromHex("#cd3700"));
	JOY_SET_EXTCOLOR(OrangeRed4, ColorFromHex("#8b2500"));
	JOY_SET_EXTCOLOR(orchid, ColorFromHex("#da70d6"));
	JOY_SET_EXTCOLOR(orchid1, ColorFromHex("#ff83fa"));
	JOY_SET_EXTCOLOR(orchid2, ColorFromHex("#ee7ae9"));
	JOY_SET_EXTCOLOR(orchid3, ColorFromHex("#cd69c9"));
	JOY_SET_EXTCOLOR(orchid4, ColorFromHex("#8b4789"));
	JOY_SET_EXTCOLOR(pale, ColorFromHex("#db7093"));
	JOY_SET_EXTCOLOR(PaleGoldenrod, ColorFromHex("#eee8aa"));
	JOY_SET_EXTCOLOR(PaleGreen, ColorFromHex("#98fb98"));
	JOY_SET_EXTCOLOR(PaleGreen1, ColorFromHex("#9aff9a"));
	JOY_SET_EXTCOLOR(PaleGreen2, ColorFromHex("#90ee90"));
	JOY_SET_EXTCOLOR(PaleGreen3, ColorFromHex("#7ccd7c"));
	JOY_SET_EXTCOLOR(PaleGreen4, ColorFromHex("#548b54"));
	JOY_SET_EXTCOLOR(PaleTurquoise, ColorFromHex("#afeeee"));
	JOY_SET_EXTCOLOR(PaleTurquoise1, ColorFromHex("#bbffff"));
	JOY_SET_EXTCOLOR(PaleTurquoise2, ColorFromHex("#aeeeee"));
	JOY_SET_EXTCOLOR(PaleTurquoise3, ColorFromHex("#96cdcd"));
	JOY_SET_EXTCOLOR(PaleTurquoise4, ColorFromHex("#668b8b"));
	JOY_SET_EXTCOLOR(PaleVioletRed, ColorFromHex("#db7093"));
	JOY_SET_EXTCOLOR(PaleVioletRed1, ColorFromHex("#ff82ab"));
	JOY_SET_EXTCOLOR(PaleVioletRed2, ColorFromHex("#ee799f"));
	JOY_SET_EXTCOLOR(PaleVioletRed3, ColorFromHex("#cd6889"));
	JOY_SET_EXTCOLOR(PaleVioletRed4, ColorFromHex("#8b475d"));
	JOY_SET_EXTCOLOR(PapayaWhip, ColorFromHex("#ffefd5"));
	JOY_SET_EXTCOLOR(PeachPuff1, ColorFromHex("#ffdab9"));
	JOY_SET_EXTCOLOR(PeachPuff2, ColorFromHex("#eecbad"));
	JOY_SET_EXTCOLOR(PeachPuff3, ColorFromHex("#cdaf95"));
	JOY_SET_EXTCOLOR(PeachPuff4, ColorFromHex("#8b7765"));
	JOY_SET_EXTCOLOR(pink, ColorFromHex("#ffc0cb"));
	JOY_SET_EXTCOLOR(pink1, ColorFromHex("#ffb5c5"));
	JOY_SET_EXTCOLOR(pink2, ColorFromHex("#eea9b8"));
	JOY_SET_EXTCOLOR(pink3, ColorFromHex("#cd919e"));
	JOY_SET_EXTCOLOR(pink4, ColorFromHex("#8b636c"));
	JOY_SET_EXTCOLOR(plum, ColorFromHex("#dda0dd"));
	JOY_SET_EXTCOLOR(plum1, ColorFromHex("#ffbbff"));
	JOY_SET_EXTCOLOR(plum2, ColorFromHex("#eeaeee"));
	JOY_SET_EXTCOLOR(plum3, ColorFromHex("#cd96cd"));
	JOY_SET_EXTCOLOR(plum4, ColorFromHex("#8b668b"));
	JOY_SET_EXTCOLOR(PowderBlue, ColorFromHex("#b0e0e6"));
	JOY_SET_EXTCOLOR(purple, ColorFromHex("#a020f0"));
	JOY_SET_EXTCOLOR(rebeccapurple, ColorFromHex("#663399"));
	JOY_SET_EXTCOLOR(purple1, ColorFromHex("#9b30ff"));
	JOY_SET_EXTCOLOR(purple2, ColorFromHex("#912cee"));
	JOY_SET_EXTCOLOR(purple3, ColorFromHex("#7d26cd"));
	JOY_SET_EXTCOLOR(purple4, ColorFromHex("#551a8b"));
	JOY_SET_EXTCOLOR(red1, ColorFromHex("#ff0000"));
	JOY_SET_EXTCOLOR(red2, ColorFromHex("#ee0000"));
	JOY_SET_EXTCOLOR(red3, ColorFromHex("#cd0000"));
	JOY_SET_EXTCOLOR(red4, ColorFromHex("#8b0000"));
	JOY_SET_EXTCOLOR(RosyBrown, ColorFromHex("#bc8f8f"));
	JOY_SET_EXTCOLOR(RosyBrown1, ColorFromHex("#ffc1c1"));
	JOY_SET_EXTCOLOR(RosyBrown2, ColorFromHex("#eeb4b4"));
	JOY_SET_EXTCOLOR(RosyBrown3, ColorFromHex("#cd9b9b"));
	JOY_SET_EXTCOLOR(RosyBrown4, ColorFromHex("#8b6969"));
	JOY_SET_EXTCOLOR(RoyalBlue, ColorFromHex("#4169e1"));
	JOY_SET_EXTCOLOR(RoyalBlue1, ColorFromHex("#4876ff"));
	JOY_SET_EXTCOLOR(RoyalBlue2, ColorFromHex("#436eee"));
	JOY_SET_EXTCOLOR(RoyalBlue3, ColorFromHex("#3a5fcd"));
	JOY_SET_EXTCOLOR(RoyalBlue4, ColorFromHex("#27408b"));
	JOY_SET_EXTCOLOR(SaddleBrown, ColorFromHex("#8b4513"));
	JOY_SET_EXTCOLOR(salmon, ColorFromHex("#fa8072"));
	JOY_SET_EXTCOLOR(salmon1, ColorFromHex("#ff8c69"));
	JOY_SET_EXTCOLOR(salmon2, ColorFromHex("#ee8262"));
	JOY_SET_EXTCOLOR(salmon3, ColorFromHex("#cd7054"));
	JOY_SET_EXTCOLOR(salmon4, ColorFromHex("#8b4c39"));
	JOY_SET_EXTCOLOR(SandyBrown, ColorFromHex("#f4a460"));
	JOY_SET_EXTCOLOR(SeaGreen1, ColorFromHex("#54ff9f"));
	JOY_SET_EXTCOLOR(SeaGreen2, ColorFromHex("#4eee94"));
	JOY_SET_EXTCOLOR(SeaGreen3, ColorFromHex("#43cd80"));
	JOY_SET_EXTCOLOR(SeaGreen4, ColorFromHex("#2e8b57"));
	JOY_SET_EXTCOLOR(seashell1, ColorFromHex("#fff5ee"));
	JOY_SET_EXTCOLOR(seashell2, ColorFromHex("#eee5de"));
	JOY_SET_EXTCOLOR(seashell3, ColorFromHex("#cdc5bf"));
	JOY_SET_EXTCOLOR(seashell4, ColorFromHex("#8b8682"));
	JOY_SET_EXTCOLOR(sienna, ColorFromHex("#a0522d"));
	JOY_SET_EXTCOLOR(sienna1, ColorFromHex("#ff8247"));
	JOY_SET_EXTCOLOR(sienna2, ColorFromHex("#ee7942"));
	JOY_SET_EXTCOLOR(sienna3, ColorFromHex("#cd6839"));
	JOY_SET_EXTCOLOR(sienna4, ColorFromHex("#8b4726"));
	JOY_SET_EXTCOLOR(SkyBlue, ColorFromHex("#87ceeb"));
	JOY_SET_EXTCOLOR(SkyBlue1, ColorFromHex("#87ceff"));
	JOY_SET_EXTCOLOR(SkyBlue2, ColorFromHex("#7ec0ee"));
	JOY_SET_EXTCOLOR(SkyBlue3, ColorFromHex("#6ca6cd"));
	JOY_SET_EXTCOLOR(SkyBlue4, ColorFromHex("#4a708b"));
	JOY_SET_EXTCOLOR(SlateBlue, ColorFromHex("#6a5acd"));
	JOY_SET_EXTCOLOR(SlateBlue1, ColorFromHex("#836fff"));
	JOY_SET_EXTCOLOR(SlateBlue2, ColorFromHex("#7a67ee"));
	JOY_SET_EXTCOLOR(SlateBlue3, ColorFromHex("#6959cd"));
	JOY_SET_EXTCOLOR(SlateBlue4, ColorFromHex("#473c8b"));
	JOY_SET_EXTCOLOR(SlateGray, ColorFromHex("#708090"));
	JOY_SET_EXTCOLOR(SlateGray1, ColorFromHex("#c6e2ff"));
	JOY_SET_EXTCOLOR(SlateGray2, ColorFromHex("#b9d3ee"));
	JOY_SET_EXTCOLOR(SlateGray3, ColorFromHex("#9fb6cd"));
	JOY_SET_EXTCOLOR(SlateGray4, ColorFromHex("#6c7b8b"));
	JOY_SET_EXTCOLOR(snow1, ColorFromHex("#fffafa"));
	JOY_SET_EXTCOLOR(snow2, ColorFromHex("#eee9e9"));
	JOY_SET_EXTCOLOR(snow3, ColorFromHex("#cdc9c9"));
	JOY_SET_EXTCOLOR(snow4, ColorFromHex("#8b8989"));
	JOY_SET_EXTCOLOR(SpringGreen1, ColorFromHex("#00ff7f"));
	JOY_SET_EXTCOLOR(SpringGreen2, ColorFromHex("#00ee76"));
	JOY_SET_EXTCOLOR(SpringGreen3, ColorFromHex("#00cd66"));
	JOY_SET_EXTCOLOR(SpringGreen4, ColorFromHex("#008b45"));
	JOY_SET_EXTCOLOR(SteelBlue, ColorFromHex("#4682b4"));
	JOY_SET_EXTCOLOR(SteelBlue1, ColorFromHex("#63b8ff"));
	JOY_SET_EXTCOLOR(SteelBlue2, ColorFromHex("#5cacee"));
	JOY_SET_EXTCOLOR(SteelBlue3, ColorFromHex("#4f94cd"));
	JOY_SET_EXTCOLOR(SteelBlue4, ColorFromHex("#36648b"));
	JOY_SET_EXTCOLOR(tan, ColorFromHex("#d2b48c"));
	JOY_SET_EXTCOLOR(tan1, ColorFromHex("#ffa54f"));
	JOY_SET_EXTCOLOR(tan2, ColorFromHex("#ee9a49"));
	JOY_SET_EXTCOLOR(tan3, ColorFromHex("#cd853f"));
	JOY_SET_EXTCOLOR(tan4, ColorFromHex("#8b5a2b"));
	JOY_SET_EXTCOLOR(thistle, ColorFromHex("#d8bfd8"));
	JOY_SET_EXTCOLOR(thistle1, ColorFromHex("#ffe1ff"));
	JOY_SET_EXTCOLOR(thistle2, ColorFromHex("#eed2ee"));
	JOY_SET_EXTCOLOR(thistle3, ColorFromHex("#cdb5cd"));
	JOY_SET_EXTCOLOR(thistle4, ColorFromHex("#8b7b8b"));
	JOY_SET_EXTCOLOR(tomato1, ColorFromHex("#ff6347"));
	JOY_SET_EXTCOLOR(tomato2, ColorFromHex("#ee5c42"));
	JOY_SET_EXTCOLOR(tomato3, ColorFromHex("#cd4f39"));
	JOY_SET_EXTCOLOR(tomato4, ColorFromHex("#8b3626"));
	JOY_SET_EXTCOLOR(turquoise, ColorFromHex("#40e0d0"));
	JOY_SET_EXTCOLOR(turquoise1, ColorFromHex("#00f5ff"));
	JOY_SET_EXTCOLOR(turquoise2, ColorFromHex("#00e5ee"));
	JOY_SET_EXTCOLOR(turquoise3, ColorFromHex("#00c5cd"));
	JOY_SET_EXTCOLOR(turquoise4, ColorFromHex("#00868b"));
	JOY_SET_EXTCOLOR(violet, ColorFromHex("#ee82ee"));
	JOY_SET_EXTCOLOR(VioletRed, ColorFromHex("#d02090"));
	JOY_SET_EXTCOLOR(VioletRed1, ColorFromHex("#ff3e96"));
	JOY_SET_EXTCOLOR(VioletRed2, ColorFromHex("#ee3a8c"));
	JOY_SET_EXTCOLOR(VioletRed3, ColorFromHex("#cd3278"));
	JOY_SET_EXTCOLOR(VioletRed4, ColorFromHex("#8b2252"));
	JOY_SET_EXTCOLOR(wheat, ColorFromHex("#f5deb3"));
	JOY_SET_EXTCOLOR(wheat1, ColorFromHex("#ffe7ba"));
	JOY_SET_EXTCOLOR(wheat2, ColorFromHex("#eed8ae"));
	JOY_SET_EXTCOLOR(wheat3, ColorFromHex("#cdba96"));
	JOY_SET_EXTCOLOR(wheat4, ColorFromHex("#8b7e66"));
	JOY_SET_EXTCOLOR(white, ColorFromHex("#ffffff"));
	JOY_SET_EXTCOLOR(WhiteSmoke, ColorFromHex("#f5f5f5"));
	JOY_SET_EXTCOLOR(yellow1, ColorFromHex("#ffff00"));
	JOY_SET_EXTCOLOR(yellow2, ColorFromHex("#eeee00"));
	JOY_SET_EXTCOLOR(yellow3, ColorFromHex("#cdcd00"));
	JOY_SET_EXTCOLOR(yellow4, ColorFromHex("#8b8b00"));
	JOY_SET_EXTCOLOR(YellowGreen, ColorFromHex("#9acd32"));
}



void InitColorsState(color_state* State, memory_region* ColorsMemory) {
    
    State->ColorsMem = ColorsMemory;
    
    InitDefaultColors(State);
    
    InitExtColors(State);
}