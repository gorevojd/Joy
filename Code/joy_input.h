#ifndef JOY_INPUT_H
#define JOY_INPUT_H

#include "joy_types.h"
#include "joy_math.h"
#include "joy_memory.h"
#include "joy_strings.h"

#define INPUT_PLATFORM_PROCESS(name) void name()
typedef INPUT_PLATFORM_PROCESS(input_platform_process);

enum move_axis_type{
    MoveAxis_Horizontal,
    MoveAxis_Vertical,
    
    MoveAxis_MouseX,
    MoveAxis_MouseY,
};

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
    
    Key_OEM3, // NOTE(Dima): It's a ` or ~
    
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

struct key_state{
    b32 EndedDown;
    b32 TransitionHappened;
    int RepeatCount;
};

struct keyboard_controller{
    key_state KeyStates[Key_Count];
};

enum gamepad_key_type{
    // NOTE(Dima): Don't change order of theese
    GamepadKey_DpadUp,
    GamepadKey_DpadDown,
    GamepadKey_DpadLeft,
    GamepadKey_DpadRight,
    
    GamepadKey_Start,
    GamepadKey_Back,
    GamepadKey_LeftThumb,
    GamepadKey_RightThumb,
    
    GamepadKey_LeftShoulder,
    GamepadKey_RightShoulder,
    
    GamepadKey_LeftTrigger,
    GamepadKey_RightTrigger,
    
    GamepadKey_A,
    GamepadKey_B,
    GamepadKey_X,
    GamepadKey_Y,
    
    GamepadKey_Count,
};

struct gamepad_key{
    key_state Key;
    
    char Name[32];
};

struct gamepad_stick{
    v2 Direction;
    float Magnitude;
};

enum gamepad_battery_charge{
    GamepadBattery_Empty,
    GamepadBattery_Low,
    GamepadBattery_Medium,
    GamepadBattery_Full,
};

struct gamepad_controller{
    gamepad_key Keys[GamepadKey_Count];
    
    gamepad_stick LeftStick;
    gamepad_stick RightStick;
    
    u32 BatteryCharge;
    
    b32 IsConnected;
};

enum button_type{
    Button_Left,
    Button_Right,
    Button_Up,
    Button_Down,
    
    Button_Jump,
    Button_Reload,
    Button_Interact,
    
    Button_OK,
    Button_Back,
    
    Button_MouseLeft,
    Button_MouseRight,
    Button_MouseUp,
    Button_MouseDown,
    
    Button_Count,
};

struct button_state{
    b32 EndedDown;
    b32 TransitionHappened;
    
#define MAX_KEYS_PER_BUTTON 4
    int Keys[MAX_KEYS_PER_BUTTON];
    int KeyCount;
    
    // NOTE(Dima): You can use it to watch how much time the key was pressed or released
    float InTransitionTime;
};

enum input_controller_source_type{
    InputcontrollerSource_None,
    
    InputControllerSource_Keyboard,
    InputControllerSource_Gamepad,
};

struct input_controller{
    button_state Buttons[Button_Count];
    
    union{
        gamepad_controller* Gamepad;
        keyboard_controller* Keyboard;
    };
    
    u32 ControllerSource;
    
    u32 GamepadIndex;
};

#define MOUSE_KEY_COUNT (Key_Count - MouseKey_Left)

struct input_state{
    // NOTE(Dima): Put here for later usage
    mem_region* Region;
    
    keyboard_controller Keyboard;
    
    float LeftStickMultiplier;
    float RightStickMultiplier;
    float KeyboardMouseMultiplier;
    
#define MAX_GAMEPAD_COUNT 4
    gamepad_controller GamepadControllers[MAX_GAMEPAD_COUNT];
    
#define MAX_CONTROLLER_COUNT 4
    input_controller Controllers[MAX_CONTROLLER_COUNT];
    
    //NOTE(Dima): In window coords top-left
    v2 MouseP;
    
    // NOTE(Dima): Use this for delta mouse and for camera rotations, etc...
    v2 MouseDeltaP;
    // NOTE(Dima): Actual is the mouse diff get from WinAPI. You should use MouseDeltaP.
    v2 MouseDeltaPActual;
    
    b32 CapturingMouse;
    b32 NotFirstFrame;
    b32 QuitRequested;
    
    char FrameInput[32];
    int FrameInputLen;
    
    f64 Time;
    f64 DeltaTime;
};

inline gamepad_controller* GetGamepad(input_state* Input, u32 GamepadIndex){
    gamepad_controller* Pad = &Input->GamepadControllers[GamepadIndex];
    
    return(Pad);
}


inline b32 KeyIsDown(input_state* input, u32 keyType){
    b32 res = input->Keyboard.KeyStates[keyType].EndedDown;
    
    return(res);
}

inline b32 KeyWentDown(input_state* input, u32 keyType){
    key_state* Key = &input->Keyboard.KeyStates[keyType];
    
    b32 res = Key->EndedDown && Key->TransitionHappened;
    
    return(res);
}

inline b32 KeyWentUp(input_state* input, u32 keyType){
    key_state* Key = &input->Keyboard.KeyStates[keyType];
    
    b32 res = !Key->EndedDown && Key->TransitionHappened;
    
    return(res);
}

// NOTE(Dima): Button gamepad events
inline b32 KeyIsDownOnPad(input_state* Input, int PadIndex, u32 PadKey){
    gamepad_controller* Pad = GetGamepad(Input, PadIndex);
    
    key_state* Key = &Pad->Keys[PadKey].Key;
    
    b32 res = Key->EndedDown;
    
    return(res);
}

inline b32 KeyWentDownOnPad(input_state* Input, int PadIndex, u32 PadKey){
    gamepad_controller* Pad = GetGamepad(Input, PadIndex);
    
    key_state* Key = &Pad->Keys[PadKey].Key;
    
    b32 res = Key->EndedDown && Key->TransitionHappened;
    
    return(res);
}

inline b32 KeyWentUpOnPad(input_state* Input, int PadIndex, u32 PadKey){
    gamepad_controller* Pad = GetGamepad(Input, PadIndex);
    
    key_state* Key = &Pad->Keys[PadKey].Key;
    
    b32 res = !Key->EndedDown && Key->TransitionHappened;
    
    return(res);
}

// NOTE(Dima): Button events on particular controller
inline b32 ButIsDownOnController(input_state* Input, int ControllerIndex, u32 ButType){
    button_state* But = &Input->Controllers[ControllerIndex].Buttons[ButType];
    
    b32 Res = But->EndedDown;
    
    return(Res);
}

inline b32 ButWentDownOnController(input_state* Input, int ControllerIndex, u32 ButType){
    button_state* But = &Input->Controllers[ControllerIndex].Buttons[ButType];
    
    b32 Res = But->EndedDown && But->TransitionHappened;
    
    return(Res);
}


inline b32 ButWentUpOnController(input_state* Input, int ControllerIndex, u32 ButType){
    button_state* But = &Input->Controllers[ControllerIndex].Buttons[ButType];
    
    b32 Res = !But->EndedDown && But->TransitionHappened;
    
    return(Res);
}

// NOTE(Dima): Button events on any of controllers
inline b32 ButIsDown(input_state* Input, u32 ButType){
    b32 Result = 0;
    
    for(int ControllerIndex = 0; 
        ControllerIndex < MAX_CONTROLLER_COUNT;
        ControllerIndex++)
    {
        input_controller* Cont = &Input->Controllers[ControllerIndex];
        
        if(ButIsDownOnController(Input, ControllerIndex, ButType)){
            Result = true;
            break;
        }
    }
    
    return(Result);
}

inline b32 ButWentDown(input_state* Input, u32 ButType){
    b32 Result = 0;
    
    for(int ControllerIndex = 0; 
        ControllerIndex < MAX_CONTROLLER_COUNT;
        ControllerIndex++)
    {
        input_controller* Cont = &Input->Controllers[ControllerIndex];
        
        if(ButWentDownOnController(Input, ControllerIndex, ButType)){
            Result = true;
            break;
        }
    }
    
    return(Result);
}

inline b32 ButWentUp(input_state* Input, u32 ButType){
    b32 Result = 0;
    
    for(int ControllerIndex = 0; 
        ControllerIndex < MAX_CONTROLLER_COUNT;
        ControllerIndex++)
    {
        input_controller* Cont = &Input->Controllers[ControllerIndex];
        
        if(ButWentUpOnController(Input, ControllerIndex, ButType)){
            Result = true;
            break;
        }
    }
    
    return(Result);
}

inline b32 MouseInRect(input_state* input, rc2 rect) {
    b32 res = 0;
    
    res =
        (input->MouseP.x >= rect.Min.x) &&
        (input->MouseP.y >= rect.Min.y) &&
        (input->MouseP.x <= rect.Max.x) &&
        (input->MouseP.y <= rect.Max.y);
    
    return(res);
}

inline b32 MouseInRect(input_state* input, v2 P, v2 Dim) {
    b32 res = 0;
    
    rc2 rect;
    rect.Min = P;
    rect.Max = P + Dim;
    
    res = MouseInRect(input, rect);
    
    return(res);
}

inline b32 MouseLeftWentDownInRect(input_state* input, rc2 rect) {
    b32 res = 0;
    
    if (MouseInRect(input, rect)) {
        if (KeyWentDown(input, MouseKey_Left)) {
            res = 1;
        }
    }
    
    return(res);
}

inline b32 MouseRightWentDownInRect(input_state* input, rc2 rect) {
    b32 res = 0;
    
    if (MouseInRect(input, rect)) {
        if (KeyWentDown(input, MouseKey_Right)) {
            res = 1;
        }
    }
    
    return(res);
}

void InitInput(input_state* Input);

v3 GetMoveVector(input_state* Input, int ControllerIndex);

float GetMoveAxisOnController(input_state* Input, int ControllerIndex, u32 Axis);
float GetMoveAxis(input_state* Input, u32 Axis);

#endif