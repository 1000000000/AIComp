#include "game_state.h"

#include <stdlib.h>

game_state* game_state_init(unsigned board_length, unsigned num_bombs, unsigned num_trails) {
	game_state* new_state = (game_state*) malloc(sizeof(game_state));
	new_state->board_length = board_length;
	new_state->num_bombs = num_bombs;
	new_state->num_trails = num_trails;
	new_state->num_refs = 1;
	new_state->bombs = (bomb*) malloc(sizeof(bomb)*num_bombs);
	new_state->trails = (trail*) malloc(sizeof(trail)*num_trails);
	new_state->hard_block_board = (int*) malloc(sizeof(int)*board_length*board_length);
	new_state->soft_block_board = (int*) malloc(sizeof(int)*board_length*board_length);
	return new_state;
}

void game_state_inc_ref(game_state* state) {
	++state->num_refs;
}

void game_state_dec_ref(game_state* state) {
	--state->num_refs;
	if (state->num_refs == 0) {
		free(state->soft_block_board);
		free(state->hard_block_board);
		free(state->trails);
		free(state->bombs);
		free(state);
	}
}

unsigned xy_to_index(game_state* state, unsigned x, unsigned y) {
	return y*state->board_length + x;
}

void index_to_xy(game_state* state, unsigned index, unsigned* x, unsigned* y) {
	*x = index % state->board_length;
	*y = index / state->board_length;
}
