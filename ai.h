#ifndef __ai_h_
#define __ai_h_

#include "game_state.h"

typedef enum {
	MOVE_UP = 0,
	MOVE_LEFT = 1,
	MOVE_DOWN = 2,
	MOVE_RIGHT = 3,
	TURN_UP = 4,
	TURN_LEFT = 5,
	TURN_DOWN = 6,
	TURN_RIGHT = 7,
	PLACE_BOMB = 8,
	ORANGE_PORTAL = 9,
	BLUE_PORTAL = 10,
	NONE = 11,
	BUY_BOMB = 12,
	BUY_PIERCE = 13,
	BUY_RANGE = 14,
	BUY_BLOCK = 15
} move_enum;

typedef struct ai_state ai_state;

ai_state* ai_state_init();

move_enum next_move(ai_state* state, game_state* gstate);

void ai_state_free(ai_state* state);

#endif//__ai_h_
