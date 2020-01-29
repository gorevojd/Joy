#ifndef JOY_MODES_H
#define JOY_MODES_H

#include "joy_game_interface.h"
#include "joy_game.h"

#define GAME_GET_MODE_STATE(state_type, state_var_name) \
state_type* state_var_name = (state_type*)Mode->ModeState;\
if(!state_var_name){\
    state_var_name = PushStruct(&Mode->Memory, state_type);\
    Mode->ModeState = state_var_name;\
}

GAME_MODE_UPDATE(TestUpdate);
GAME_MODE_UPDATE(MainMenuUpdate);
GAME_MODE_UPDATE(ChangingPicturesUpdate);
GAME_MODE_UPDATE(TitleUpdate);

#endif