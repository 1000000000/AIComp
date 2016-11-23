#include "ai.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct ai_state {
	int uninit;
	unsigned* dists, *thresh;
} ai_state;

ai_state* ai_state_init() {
	ai_state* new_state = (ai_state*) malloc(sizeof(ai_state));
	new_state->uninit = 1;
	return new_state;
}

unsigned num_cells(game_state* gs) {
	return gs->board_length*gs->board_length;
}

void print_array(game_state* gs, unsigned* arr) {
	unsigned i;
	for (i = 0; i < num_cells(gs); ++i) {
		if (arr[i] == -1) {
			printf("X");
		} else {
			printf("%u", arr[i]);
		}
		if ((i + 1) % gs->board_length == 0) {
			printf("\n");
		}
	}
}

void first_run_init(ai_state* state, game_state* gs) {
	state->uninit = 0;
	state->dists = (unsigned*) malloc(num_cells(gs)*sizeof(unsigned));
	state->thresh = (unsigned*) malloc(num_cells(gs)*sizeof(unsigned));
}

int is_bomb_or_trail(game_state* gs, unsigned x, unsigned y) {
	unsigned i;
	for (i = 0; i < gs->num_bombs; ++i) {
		if (gs->bombs[i].x == x && gs->bombs[i].y == y) {
			return 1;
		}
	}
	for (i = 0; i < gs->num_trails; ++i) {
		if (gs->trails[i].x == x && gs->trails[i].y == y) {
			return 1;
		}
	}
	return 0;
}

int is_walkable(game_state* gs, unsigned x, unsigned y) {
	return !gs->hard_block_board[xy_to_index(gs,x,y)]
		&& !gs->soft_block_board[xy_to_index(gs,y,x)]
		&& !is_bomb_or_trail(gs, x, y)
		&& gs->opponent.x != x && gs->opponent.y != y;
}

void add_to_queue(unsigned* dists, unsigned* thresh, game_state* gs, unsigned x, unsigned y, unsigned* queue, unsigned start, unsigned* end) {
	unsigned index = xy_to_index(gs, x, y);
	if (dists[index] == -1 && dists[queue[start]] + 1 < thresh[index] && is_walkable(gs,x,y)) {
		dists[index] = dists[queue[start]] + 1;
		queue[*end] = index;
		*end = *end + 1;
	}
}

void bfs(unsigned* dists, unsigned* thresh, game_state* gs) {
	unsigned i, x, y, *queue, end, start;
	for (i = 0; i < num_cells(gs); ++i) {
		dists[i] = -1;
	}
	queue = (unsigned*) malloc(num_cells(gs)*sizeof(unsigned));
	x = gs->self.x;
	y = gs->self.y;
	queue[0] = xy_to_index(gs,x,y);
	dists[queue[0]] = 0;
	start = 0;
	end = 1;
	while (start != end) {
		index_to_xy(gs, queue[start], &x, &y);
		if (x > 0) {
			add_to_queue(dists, thresh, gs, x-1, y, queue, start, &end);
		}
		if (x + 1 < gs->board_length) {
			add_to_queue(dists, thresh, gs, x+1, y, queue, start, &end);
		}
		if (y > 0) {
			add_to_queue(dists, thresh, gs, x, y-1, queue, start, &end);
		}
		if (y + 1 < gs->board_length) {
			add_to_queue(dists, thresh, gs, x, y+1, queue, start, &end);
		}
		++start;
	}
	free(queue);
}

unsigned diff(unsigned a, unsigned b) {
	if (a > b) {
		return a - b;
	} else {
		return b - a;
	}
}

unsigned min(unsigned a, unsigned b) {
	if (a > b) {
		return b;
	} else {
		return a;
	}
}

void place_future_trails(unsigned* thresh, bomb* bomb, game_state* gs, unsigned* axis, int dir) {
	unsigned true_range = bomb->range, backup = *axis;
	while (diff(*axis,backup) < true_range && ((dir == -1 && *axis > 0) || (dir == 1 && *axis + 1 < gs->board_length))) {
		*axis += dir;
		if (gs->hard_block_board[xy_to_index(gs,bomb->x,bomb->y)] || gs->soft_block_board[xy_to_index(gs,bomb->y,bomb->x)]) {
			true_range = min(true_range, diff(*axis,backup) + bomb->pierce);
		} else if (thresh[xy_to_index(gs,bomb->x,bomb->y)] > bomb->tick) {
			thresh[xy_to_index(gs,bomb->x,bomb->y)] = bomb->tick;
		}
	}
	*axis = backup;
}

void bomb_thresholds(unsigned* thresh, game_state* gs) {
	unsigned i;
	for (i = 0; i < num_cells(gs); ++i) {
		thresh[i] = -1;
	}
	for (i = 0; i < gs->num_bombs; ++i) {
		thresh[xy_to_index(gs, gs->bombs[i].x, gs->bombs[i].y)] = 0;
		place_future_trails(thresh, &gs->bombs[i], gs, &gs->bombs[i].x, 1);
		place_future_trails(thresh, &gs->bombs[i], gs, &gs->bombs[i].x, -1);
		place_future_trails(thresh, &gs->bombs[i], gs, &gs->bombs[i].y, 1);
		place_future_trails(thresh, &gs->bombs[i], gs, &gs->bombs[i].y, -1);
	}
}

unsigned taxicab(unsigned x1, unsigned y1, unsigned x2, unsigned y2) {
	return diff(x1,x2) + diff(y1,y2);
}

move_enum next_step_to_dest(unsigned* dists, game_state* gs, unsigned dx, unsigned dy) {
	unsigned cur_dist;
	while (taxicab(gs->self.x, gs->self.y, dx, dy) > 1) {
		cur_dist = dists[xy_to_index(gs,dx,dy)];
		if (dx + 1 < gs->board_length && dists[xy_to_index(gs,dx+1,dy)] < cur_dist) {
			++dx;
		} else if (dy + 1 < gs->board_length && dists[xy_to_index(gs,dx,dy+1)] < cur_dist) {
			++dy;
		} else if (dx > 0 && dists[xy_to_index(gs,dx-1,dy)] < cur_dist) {
			--dx;
		} else if (dy > 0 && dists[xy_to_index(gs,dx,dy-1)] < cur_dist) {
			--dy;
		} else {
			return TURN_UP;
		}
	}
	if (diff(gs->self.x, dx) > 0) {
		if (gs->self.x < dx) {
			return MOVE_RIGHT;
		} else {
			return MOVE_LEFT;
		}
	} else if (diff(gs->self.y, dy) > 0) {
		if (gs->self.y < dy) {
			return MOVE_DOWN;
		} else {
			return MOVE_UP;
		}
	} else {
		return TURN_LEFT;
	}
}

move_enum next_move(ai_state* state, game_state* gs) {
	unsigned i, x, y, bestx = -1, besty = -1, best_score = -1;
	if (state->uninit) {
		first_run_init(state, gs);
	}
	bomb_thresholds(state->thresh, gs);
#ifdef DEBUG
	print_array(gs, state->thresh);
#endif
	if (gs->self.bomb_count_cur == 0 && state->thresh[xy_to_index(gs,gs->self.x,gs->self.y)] == -1) {
		return TURN_DOWN;
	}
	bfs(state->dists, state->thresh, gs);
#ifdef DEBUG
	print_array(gs, state->dists);
#endif
	for (i = 0; i < num_cells(gs); ++i) {
		if (gs->self.bomb_count_cur == 0) {
			if (state->thresh[i] == -1 && state->dists[i] < best_score) {
				best_score = state->dists[i];
				index_to_xy(gs,i,&bestx,&besty);
			}
		} else {
			if (state->dists[i] != -1) {
				index_to_xy(gs,i,&x,&y);
				if (taxicab(x,y,gs->opponent.x,gs->opponent.y) < best_score) {
					best_score = taxicab(x,y,gs->opponent.x,gs->opponent.y);
					bestx = x;
					besty = y;
				}
			}
		}
	}
#ifdef DEBUG
	if (gs->self.bomb_count_cur == 0) {
		printf("Strategy: take cover\n");
	} else {
		printf("Strategy: place bomb\n");
	}
	printf("Destination: (%u,%u)\n", bestx, besty);
#endif
	if (bestx == -1 || besty == -1) {
		return TURN_RIGHT;
	}
	if (gs->self.bomb_count_cur != 0 && gs->self.x == bestx && gs->self.y == besty) {
		return PLACE_BOMB;
	}
	return next_step_to_dest(state->dists, gs, bestx, besty);
}

void ai_state_free(ai_state* state) {
	if (state->thresh != NULL) {
		free(state->thresh);
	}
	if (state->dists != NULL) {
		free(state->dists);
	}
	free(state);
}
