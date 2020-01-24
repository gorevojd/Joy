#ifndef JOY_INPUT_H
#define JOY_INPUT_H

#include "joy_types.h"
#include "joy_math.h"

enum KeyType{
    KeyMouse_Left,
    KeyMouse_Right,
    KeyMouse_Middle,
    KeyMouse_X1,
    KeyMouse_X2,
    
    Key_Up,
    Key_Down,
    Key_Left,
    Key_Right,
    
    Key_Backspace,
    Key_Tab,
    Key_Return,
    Key_Shift,
    Key_Control,
    Key_Escape,
    Key_Space,
    Key_Home,
    Key_End,
    Key_Insert,
    Key_Delete,
    Key_Help,
    
    Key_0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,
    
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,
    
    Key_Num0,
    Key_Num1,
    Key_Num2,
    Key_Num3,
    Key_Num4,
    Key_Num5,
    Key_Num6,
    Key_Num7,
    Key_Num8,
    Key_Num9,
    
    Key_Multiply,
    Key_Add,
    Key_Divide,
    Key_Subtract,
    Key_Separator,
    Key_Decimal,
    
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,
    
    Key_VolumeMute,
    Key_VolumeUp,
    Key_VolumeDown,
    
    // NOTE(Dima): Do not change order after this line
    MouseKey_Left,
    MouseKey_Middle,
    MouseKey_Right,
    MouseKey_Extended1,
    MouseKey_Extended2,
    
    Key_Count,
};

#define MOUSE_KEY_COUNT (Key_Count - MouseKey_Left)

struct KeyState{
    b32 endedDown;
    b32 transitionHappened;
    int RepeatCount;
};

struct Input{
    KeyState KeyStates[Key_Count];
    
    //NOTE(Dima): In window coords top-left
    v2 LastMouseP;
    v2 MouseP;
    
    char FrameInput[32];
    int FrameInputLen;
};

inline b32 KeyIsDown(Input* input, u32 keyType){
    b32 res = input->KeyStates[keyType].endedDown;
    
    return(res);
}

inline b32 KeyWentDown(Input* input, u32 keyType){
    KeyState* Key = &input->KeyStates[keyType];
    
    b32 res = Key->endedDown && Key->transitionHappened;
    
    return(res);
}

inline b32 KeyWentUp(Input* input, u32 keyType){
    KeyState* Key = &input->KeyStates[keyType];
    
    b32 res = !Key->endedDown && Key->transitionHappened;
    
    return(res);
}

inline b32 MouseInRect(Input* input, rc2 rect) {
	b32 res = 0;
    
	res =
		(input->MouseP.x >= rect.min.x) &&
		(input->MouseP.y >= rect.min.y) &&
		(input->MouseP.x <= rect.max.x) &&
		(input->MouseP.y <= rect.max.y);
    
	return(res);
}

inline b32 MouseInRect(Input* input, v2 P, v2 Dim) {
	b32 res = 0;
    
	rc2 rect;
	rect.min = P;
	rect.max = P + Dim;
    
    res = MouseInRect(input, rect);
    
    return(res);
}

inline b32 MouseLeftWentDownInRect(Input* input, rc2 rect) {
	b32 res = 0;
    
	if (MouseInRect(input, rect)) {
		if (KeyWentDown(input, MouseKey_Left)) {
			res = 1;
		}
	}
    
	return(res);
}

inline b32 MouseRightWentDownInRect(Input* input, rc2 rect) {
	b32 res = 0;
    
	if (MouseInRect(input, rect)) {
		if (KeyWentDown(input, MouseKey_Right)) {
			res = 1;
		}
	}
    
	return(res);
}


#endif