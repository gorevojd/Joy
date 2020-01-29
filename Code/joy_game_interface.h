#ifndef JOY_GAME_INTERFACE_H
#define JOY_GAME_INTERFACE_H

#include "joy_memory.h"

#define GAME_MODE_UPDATE(name) void name(struct game_state* Game, struct game_mode* Mode)
typedef GAME_MODE_UPDATE(game_mode_update_prototype);

#define GAME_MODE_ENDFRAME(name) void name(struct game_state* Game, struct game_mode* Mode)
typedef GAME_MODE_ENDFRAME(game_mode_endframe_prototype);

#endif