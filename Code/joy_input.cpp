#include "joy_input.h"


INTERNAL_FUNCTION inline void _AddKeyToButOnCont(input_controller* Cont, u32 Button, u32 BoardOrPadKey){
    ASSERT(MAX_KEYS_PER_BUTTON > Cont->Buttons[Button].KeyCount);
    
    Cont->Buttons[Button].Keys[Cont->Buttons[Button].KeyCount++] = BoardOrPadKey;
}

INTERNAL_FUNCTION inline void AddKeyToButtonOnController(input_state* Input, int ControllerIndex, 
                                                         u32 Button, u32 BoardOrPadKey)
{
    input_controller* Cont = &Input->Controllers[ControllerIndex];
    
    _AddKeyToButOnCont(Cont, Button, BoardOrPadKey);
}

INTERNAL_FUNCTION inline void MapGamepadOnInputController(input_state* Input, int ControllerIndex, int GamepadIndex)
{
    input_controller* Cont = &Input->Controllers[ControllerIndex];
    
    Cont->ControllerSource = InputControllerSource_Gamepad;
    Cont->GamepadIndex = GamepadIndex;
    
    _AddKeyToButOnCont(Cont, Button_Down, GamepadKey_DpadDown);
    _AddKeyToButOnCont(Cont, Button_Up, GamepadKey_DpadUp);
    _AddKeyToButOnCont(Cont, Button_Left, GamepadKey_DpadLeft);
    _AddKeyToButOnCont(Cont, Button_Right, GamepadKey_DpadRight);
}


void InitInput(input_state* Input)
{
    for(int ControllerIndex = 0; 
        ControllerIndex < MAX_CONTROLLER_COUNT;
        ControllerIndex++)
    {
        Input->Controllers[ControllerIndex].ControllerSource = InputcontrollerSource_None;
    }
    
    for(int GamepadIndex = 0; 
        GamepadIndex < MAX_GAMEPAD_COUNT;
        GamepadIndex++)
    {
        gamepad_controller* Pad = &Input->GamepadControllers[GamepadIndex];
        
        CopyStrings(Pad->Keys[GamepadKey_DpadUp].Name, "DpadUp");
        CopyStrings(Pad->Keys[GamepadKey_DpadDown].Name, "DpadDown");
        CopyStrings(Pad->Keys[GamepadKey_DpadLeft].Name, "DpadLeft");
        CopyStrings(Pad->Keys[GamepadKey_DpadRight].Name, "DpadRight");
        CopyStrings(Pad->Keys[GamepadKey_Start].Name, "Start");
        CopyStrings(Pad->Keys[GamepadKey_Back].Name, "Back");
        CopyStrings(Pad->Keys[GamepadKey_LeftThumb].Name, "LeftThumb");
        CopyStrings(Pad->Keys[GamepadKey_RightThumb].Name, "RightThumb");
        CopyStrings(Pad->Keys[GamepadKey_LeftShoulder].Name, "LeftShoulder");
        CopyStrings(Pad->Keys[GamepadKey_RightShoulder].Name, "RightShoulder");
        CopyStrings(Pad->Keys[GamepadKey_A].Name, "A");
        CopyStrings(Pad->Keys[GamepadKey_B].Name, "B");
        CopyStrings(Pad->Keys[GamepadKey_X].Name, "X");
        CopyStrings(Pad->Keys[GamepadKey_Y].Name, "Y");
    }
    
    Input->Controllers[0].ControllerSource = InputControllerSource_Keyboard;
    Input->Controllers[1].ControllerSource = InputControllerSource_Keyboard;
    
    MapGamepadOnInputController(Input, 2, 0);
    MapGamepadOnInputController(Input, 3, 1);
    MapGamepadOnInputController(Input, 4, 2);
    MapGamepadOnInputController(Input, 5, 3);
    
    AddKeyToButtonOnController(Input, 0, Button_Left, Key_A);
    AddKeyToButtonOnController(Input, 0, Button_Right, Key_D);
    AddKeyToButtonOnController(Input, 0, Button_Up, Key_W);
    AddKeyToButtonOnController(Input, 0, Button_Down, Key_S);
    AddKeyToButtonOnController(Input, 0, Button_Jump, Key_Space);
    AddKeyToButtonOnController(Input, 0, Button_Reload, Key_R);
    AddKeyToButtonOnController(Input, 0, Button_Interact, Key_F);
    
    // TODO(Dima): Add some others for this controller
    AddKeyToButtonOnController(Input, 1, Button_Left, Key_Left);
    AddKeyToButtonOnController(Input, 1, Button_Right, Key_Right);
    AddKeyToButtonOnController(Input, 1, Button_Up, Key_Up);
    AddKeyToButtonOnController(Input, 1, Button_Down, Key_Down);
    AddKeyToButtonOnController(Input, 1, Button_Interact, Key_Return);
}


// NOTE(Dima): If ControllerIndex passed as -1 then all controllers info collected
v3 GetMoveVector(input_state* Input, int ControllerIndex){
    v3 MoveVector = {};
    
    input_controller* Cont = 0;
    
    if(ControllerIndex >= 0){
        Cont = &Input->Controllers[ControllerIndex];
    }
    
    if(Cont){
        if(Cont->ControllerSource == InputControllerSource_Gamepad){
            gamepad_stick* PadStick = &Input->GamepadControllers[Cont->GamepadIndex].LeftStick;
            v2 Stick = PadStick->Direction * PadStick->Magnitude;
            
            MoveVector = V3(-Stick.x, 0.0f, Stick.y);
        }
        else if(Cont->ControllerSource == InputControllerSource_Keyboard){
            if(ButIsDownOnController(Input, ControllerIndex, Button_Left))
            {
                MoveVector.x += 1.0f;
            }
            
            if(ButIsDownOnController(Input, ControllerIndex, Button_Right))
            {
                MoveVector.x += -1.0f;
            }
            
            if(ButIsDownOnController(Input, ControllerIndex, Button_Up))
            {
                MoveVector.z += 1.0f;
            }
            
            if(ButIsDownOnController(Input, ControllerIndex, Button_Down))
            {
                MoveVector.z += -1.0f;
            }
        }
    }
    else{
        v2 SumStickMove = {};
        for(int ContIndex = 0; ContIndex < MAX_CONTROLLER_COUNT; ContIndex++){
            
            input_controller* Cont = &Input->Controllers[ContIndex];
            
            if(Cont->ControllerSource == InputControllerSource_Gamepad){
                
                gamepad_controller* Pad = GetGamepad(Input, Cont->GamepadIndex);
                
                SumStickMove += Pad->LeftStick.Direction;
            }
        }
        
        SumStickMove = NOZ(SumStickMove);
        
        MoveVector = V3(-SumStickMove.x, 0.0f, SumStickMove.y);
        
        if(ButIsDown(Input, Button_Left))
        {
            MoveVector.x += 1.0f;
        }
        
        if(ButIsDown(Input, Button_Right))
        {
            MoveVector.x += -1.0f;
        }
        
        if(ButIsDown(Input, Button_Up))
        {
            MoveVector.z += 1.0f;
        }
        
        if(ButIsDown(Input, Button_Down))
        {
            MoveVector.z += -1.0f;
        }
    }
    
    MoveVector = NOZ(MoveVector);
    MoveVector = -MoveVector;
    
    return(MoveVector);
}
