#ifndef JOY_GAME_MODE_H
#define JOY_GAME_MODE_H

#include "joy_platform.h"
#include "joy_types.h"

#define GAME_MODE_UPDATE(name) void name()
typedef GAME_MODE_UPDATE(game_mode_update_prototype);

#define GAME_MODE_ENDFRAME(name) void name()
typedef GAME_MODE_ENDFRAME(game_mode_endframe_prototype);

struct game_mode_interface{
    char* Name;
    game_mode_update_prototype* Update;
    game_mode_endframe_prototype* EndFrame;
};

#endif