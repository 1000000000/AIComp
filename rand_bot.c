#include "ai.h"

#include <stdlib.h>
#include <time.h>

typedef struct ai_state {} ai_state;

ai_state* ai_state_init() {
	srand(time(NULL));
	return NULL;
}

move_enum next_move(ai_state* state, json_object* current_board) {
	return rand() % 16;
}

void ai_state_free(ai_state* state) {}
