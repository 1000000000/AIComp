#ifndef __game_state_h_
#define __game_state_h_

typedef enum {
	LEFT, UP, RIGHT, DOWN
} direction;

typedef struct trail {
	unsigned x, y, tick;
} trail;

typedef struct bomb {
	int ours; // bool
	unsigned x, y, tick, range, pierce;
} bomb;

typedef struct portal {
	unsigned x, y;
	direction dir;
} portal;

typedef struct player {
	portal blue, orange;
	direction orientation;
	unsigned bomb_range, bomb_count_max, bomb_count_cur, bomb_pierce, coins, x, y;
} player;

typedef struct game_state {
	int* hard_block_board, *soft_block_board;
	unsigned board_length, num_bombs, num_trails, num_refs;
	int turn_start; // bool
	player self, opponent;
	bomb* bombs;
	trail* trails;
} game_state;

game_state* game_state_init(unsigned board_length, unsigned num_bombs, unsigned num_trails);

void game_state_inc_ref(game_state* state);

void game_state_dec_ref(game_state* state);

unsigned xy_to_index(game_state* state, unsigned x, unsigned y);

void index_to_xy(game_state* state, unsigned index, unsigned* x, unsigned* y);

#endif//__game_state_h_
