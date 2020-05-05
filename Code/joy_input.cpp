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
    
    Cont->ControllerSourceType = InputControllerSource_Gamepad;
    Cont->GamepadIndex = GamepadIndex;
    
    _AddKeyToButOnCont(Cont, Button_Down, GamepadKey_DpadDown);
    _AddKeyToButOnCont(Cont, Button_Up, GamepadKey_DpadUp);
    _AddKeyToButOnCont(Cont, Button_Left, GamepadKey_DpadLeft);
    _AddKeyToButOnCont(Cont, Button_Right, GamepadKey_DpadRight);
}

INTERNAL_FUNCTION inline float GetMoveValueFromButtons(input_state* Input, 
                                                       int ControllerIndex, 
                                                       u32 Axis, 
                                                       b32* OutGot)
{
    b32 InternalGot = 0;
    
    float Result = 0.0f;
    
    float MoveValue = 0.0f;
    if(Axis == MoveAxis_Horizontal){
        if(ButIsDownOnController(Input, ControllerIndex, Button_Left))
        {
            MoveValue += 1.0f;
            InternalGot = true;
        }
        
        if(ButIsDownOnController(Input, ControllerIndex, Button_Right))
        {
            MoveValue += -1.0f;
            InternalGot = true;
        }
    }
    else if(Axis == MoveAxis_Vertical){
        if(ButIsDownOnController(Input, ControllerIndex, Button_Up))
        {
            MoveValue += 1.0f;
            InternalGot = true;
        }
        
        if(ButIsDownOnController(Input, ControllerIndex, Button_Down))
        {
            MoveValue += -1.0f;
            InternalGot = true;
        }
        
    }
    else if(Axis == MoveAxis_MouseX){
        if(ButIsDownOnController(Input, ControllerIndex, Button_MouseLeft))
        {
            MoveValue += 1.0f;
            InternalGot = true;
        }
        
        if(ButIsDownOnController(Input, ControllerIndex, Button_MouseRight))
        {
            MoveValue += -1.0f;
            InternalGot = true;
        }
    }
    else if(Axis == MoveAxis_MouseY){
        if(ButIsDownOnController(Input, ControllerIndex, Button_MouseUp))
        {
            MoveValue += 1.0f;
            InternalGot = true;
        }
        
        if(ButIsDownOnController(Input, ControllerIndex, Button_MouseDown))
        {
            MoveValue += -1.0f;
            InternalGot = true;
        }
    }
    
    if(OutGot){
        *OutGot = InternalGot;
    }
    Result = MoveValue;
    
    return(Result);
}

INTERNAL_FUNCTION inline float _GetMoveAxisOnController(input_state* Input, input_controller* Cont, int ControllerIndex, u32 Axis, b32* OutIfGot)
{
    float Result = 0.0f;
    
    b32 Got = false;
    
    if(!Cont){
        Cont = &Input->Controllers[ControllerIndex];
    }
    
    if(Axis == MoveAxis_Horizontal ||
       Axis == MoveAxis_Vertical)
    {
        switch(Cont->ControllerSourceType){
            case InputControllerSource_Gamepad:{
                gamepad_stick* PadStick = &Input->GamepadControllers[Cont->GamepadIndex].LeftStick;
                
                if(Axis == MoveAxis_Horizontal){
                    Result = -PadStick->Direction.x * PadStick->Magnitude * Input->LeftStickMultiplier;
                }
                else if(Axis == MoveAxis_Vertical){
                    Result = PadStick->Direction.y * PadStick->Magnitude * Input->LeftStickMultiplier;
                }
                
                b32 InternalGot = false;
                float MoveValue = GetMoveValueFromButtons(Input, 
                                                          ControllerIndex, 
                                                          Axis, 
                                                          &InternalGot);
                if(InternalGot){
                    Result = MoveValue;
                }
                
                Got = true;
            }break;
            
            case InputControllerSource_Keyboard:{
                float MoveValue = GetMoveValueFromButtons(Input, ControllerIndex, Axis, &Got);
                
                if(Got){
                    Result = MoveValue;
                }
            }break;
        }
    }
    else if ((Axis == MoveAxis_MouseX) ||
             (Axis == MoveAxis_MouseY))
    {
        switch(Cont->ControllerSourceType){
            case InputControllerSource_Gamepad:{
                gamepad_stick* PadStick = &Input->GamepadControllers[Cont->GamepadIndex].RightStick;
                
                if(PadStick->Magnitude > 0.0f){
                    if(Axis == MoveAxis_MouseX){
                        Result = -PadStick->Direction.x * PadStick->Magnitude * Input->RightStickMultiplier;
                    }
                    else if(Axis == MoveAxis_MouseY){
                        Result = -PadStick->Direction.y * PadStick->Magnitude * Input->RightStickMultiplier;
                    }
                    
                    Got = true;
                }
                
            }break;
            
            case InputControllerSource_Keyboard:{
                
                if((Axis == MoveAxis_MouseX) && (Abs(Input->MouseDeltaP.x) > 0.0f)){
                    Result = Input->MouseDeltaP.x;
                    
                    Got = true;
                }
                else if((Axis == MoveAxis_MouseY) && (Abs(Input->MouseDeltaP.y) > 0.0f)){
                    Result = Input->MouseDeltaP.y;
                    
                    Got = true;
                }
                
                b32 InternalGot = false;
                float ButMove = GetMoveValueFromButtons(Input, ControllerIndex, Axis, &InternalGot);
                
                if(InternalGot){
                    Result += ButMove * Input->KeyboardMouseMultiplier;
                    Got = true;
                }
            }break;
        }
    }
    
    if(OutIfGot){
        *OutIfGot = Got;
    }
    
    return(Result);
}

float GetMoveAxisOnController(input_state* Input, int ControllerIndex, u32 Axis){
    input_controller* Cont = &Input->Controllers[ControllerIndex];
    
    float Result = _GetMoveAxisOnController(Input, Cont, ControllerIndex, Axis, 0);
    
    return(Result);
}

float GetMoveAxis(input_state* Input, u32 Axis){
    float Result = 0.0f;
    
    for(int ControllerIndex = 0; ControllerIndex < MAX_CONTROLLER_COUNT; ControllerIndex++){
        input_controller* Cont = &Input->Controllers[ControllerIndex];
        
        b32 Got = false;
        float MoveValue = _GetMoveAxisOnController(Input, Cont, ControllerIndex, Axis, &Got);
        if(Got){
            Result = MoveValue;
            break;
        }
    }
    
    return(Result);
}

void InitInput(input_state* Input)
{
    Input->CapturingMouse = true;
    
    Input->LeftStickMultiplier = 1.0f;
    Input->RightStickMultiplier = 1.0f;
    Input->KeyboardMouseMultiplier = 1.0f;
    
    for(int ControllerIndex = 0; 
        ControllerIndex < MAX_CONTROLLER_COUNT;
        ControllerIndex++)
    {
        Input->Controllers[ControllerIndex].ControllerSourceType = InputcontrollerSource_None;
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
        CopyStrings(Pad->Keys[GamepadKey_LeftTrigger].Name, "LeftTrigger");
        CopyStrings(Pad->Keys[GamepadKey_RightTrigger].Name, "RightTrigger");
        CopyStrings(Pad->Keys[GamepadKey_A].Name, "A");
        CopyStrings(Pad->Keys[GamepadKey_B].Name, "B");
        CopyStrings(Pad->Keys[GamepadKey_X].Name, "X");
        CopyStrings(Pad->Keys[GamepadKey_Y].Name, "Y");
    }
    
    Input->Controllers[0].ControllerSourceType = InputControllerSource_Keyboard;
    Input->Controllers[1].ControllerSourceType = InputControllerSource_Keyboard;
    
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
    AddKeyToButtonOnController(Input, 1, Button_MouseLeft, Key_Left);
    AddKeyToButtonOnController(Input, 1, Button_MouseRight, Key_Right);
    AddKeyToButtonOnController(Input, 1, Button_MouseUp, Key_Up);
    AddKeyToButtonOnController(Input, 1, Button_MouseDown, Key_Down);
    
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
    
    if((ControllerIndex >= 0) && 
       (ControllerIndex < MAX_CONTROLLER_COUNT))
    {
        Cont = &Input->Controllers[ControllerIndex];
    }
    
    if(Cont){
        float Horizontal = GetMoveAxisOnController(
            Input, 
            ControllerIndex, 
            MoveAxis_Horizontal);
        float Vertical = GetMoveAxisOnController(
            Input, 
            ControllerIndex, 
            MoveAxis_Vertical);
        
        MoveVector = V3(Horizontal, 0.0f, Vertical);
    }
    else{
        
        float Horizontal = GetMoveAxis(Input, MoveAxis_Horizontal);
        float Vertical = GetMoveAxis(Input, MoveAxis_Vertical);
        
        MoveVector = V3(Horizontal, 0.0f, Vertical);
    }
    
    MoveVector = NOZ(MoveVector);
    MoveVector = -MoveVector;
    
    return(MoveVector);
}
