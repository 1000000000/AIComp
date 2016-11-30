#include "ai.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct ai_state {
	int uninit, done_mining;
	unsigned* dists, *thresh;
} ai_state;

ai_state* ai_state_init() {
	ai_state* new_state = (ai_state*) malloc(sizeof(ai_state));
	new_state->uninit = 1;
	new_state->done_mining = 0;
	return new_state;
}

unsigned diff(unsigned a, unsigned b) {
	if (a > b) {
		return a - b;
	} else {
		return b - a;
	}
}

unsigned max(unsigned a, unsigned b) {
	if (a > b) {
		return a;
	} else {
		return b;
	}
}

unsigned min(unsigned a, unsigned b) {
	if (a > b) {
		return b;
	} else {
		return a;
	}
}

unsigned num_cells(game_state* gs) {
	return gs->board_length*gs->board_length;
}

void print_array(game_state* gs, unsigned* arr) {
	unsigned i;
	printf("\n");
	for (i = 0; i < num_cells(gs); ++i) {
		if (arr[i] == -1) {
			printf(".");
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

int is_bomb(game_state* gs, unsigned x, unsigned y) {
	unsigned i;
	for (i = 0; i < gs->num_bombs; ++i) {
		if (gs->bombs[i].x == x && gs->bombs[i].y == y) {
			return 1;
		}
	}
	return 0;
}

int is_wall(game_state* gs, unsigned x, unsigned y) {
	return gs->hard_block_board[xy_to_index(gs,x,y)]
		|| gs->soft_block_board[xy_to_index(gs,x,y)];
}

int is_walkable(game_state* gs, unsigned x, unsigned y) {
	return !is_wall(gs, x, y)
		&& !is_bomb(gs, x, y)
		&& gs->opponent.x != x && gs->opponent.y != y;
}

unsigned trail_tick(game_state* gs, unsigned x, unsigned y) {
	unsigned i;
	for (i = 0; i < gs->num_trails; ++i) {
		if (gs->trails[i].x == x && gs->trails[i].y == y) {
			return gs->trails[i].tick + 1;
		}
	}
	return 0;
}

int exists_path(game_state* gs, int* visited, unsigned sx, unsigned sy, unsigned tx, unsigned ty) {
	int ret;
	int* vds;
	if (is_wall(gs, tx, ty) || is_wall(gs, sx, sy)) {
		return 0;
	}
	if (sx == tx && sy == ty) {
		return 1;
	}
	if (visited == NULL) {
		vds = (int*) calloc(num_cells(gs), sizeof(int));
	} else if (visited[xy_to_index(gs, sx, sy)]) {
		return 0;
	} else {
		vds = visited;
	}
	vds[xy_to_index(gs,sx,sy)] = 1;
	ret =  (sx > 0 && exists_path(gs, vds, sx - 1, sy, tx, ty))
		|| (sy > 0 && exists_path(gs, vds, sx, sy - 1, tx, ty))
		|| (sx + 1 < gs->board_length && exists_path(gs, vds, sx + 1, sy, tx, ty))
		|| (sy + 1 < gs->board_length && exists_path(gs, vds, sx, sy + 1, tx, ty));
	vds[xy_to_index(gs,sx,sy)] = 0;
	if (visited == NULL) {
		free(vds);
	}
	return ret;
}

void add_to_queue(unsigned* dists, unsigned* thresh, game_state* gs, unsigned x, unsigned y, unsigned* queue, unsigned start, unsigned* end) {
	unsigned index = xy_to_index(gs, x, y), new_dist = max(dists[queue[start]] + 1, trail_tick(gs, x, y));
	if (dists[index] > new_dist && new_dist < thresh[index] && is_walkable(gs,x,y)) {
		if (dists[index] == -1) {
			queue[*end] = index;
			*end = *end + 1;
		}
		dists[index] = new_dist;
	}
}

void bfs(unsigned* dists, unsigned* thresh, game_state* gs) {
	unsigned i, x, y, *queue, end, start, best, best_ind;
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
		best = dists[queue[start]];
		best_ind = start;
		for (i = start + 1; i < end; ++i) {
			if (dists[queue[i]] < best) {
				best = dists[queue[i]];
				best_ind = i;
			}
		}
		index_to_xy(gs, queue[best_ind], &x, &y);
		if (x > 0) {
			add_to_queue(dists, thresh, gs, x-1, y, queue, best_ind, &end);
		}
		if (x + 1 < gs->board_length) {
			add_to_queue(dists, thresh, gs, x+1, y, queue, best_ind, &end);
		}
		if (y > 0) {
			add_to_queue(dists, thresh, gs, x, y-1, queue, best_ind, &end);
		}
		if (y + 1 < gs->board_length) {
			add_to_queue(dists, thresh, gs, x, y+1, queue, best_ind, &end);
		}
		queue[best_ind] = queue[start];
		++start;
	}
	free(queue);
}

direction rev_trail_direction(bomb* bomb, unsigned* axis, int dir) {
	direction ret;
	if (&bomb->x == axis) {
		ret = 1;
	} else {
		ret = 2;
	}
	return ret - dir;
}

void direction_to_axis_dir(direction d, bomb* bomb, unsigned** axis, int* dir) {
	if (d % 2 == 0) {
		*axis = &bomb->x;
	} else {
		*axis = &bomb->y;
	}
	if (d < 2) {
		*dir = -1;
	} else {
		*dir = 1;
	}

}

int is_portal(game_state* gs, unsigned x, unsigned y, direction d) {
	return (gs->opponent.orange.x == x && gs->opponent.orange.y == y && gs->opponent.orange.dir == d)
			|| (gs->opponent.blue.x == x && gs->opponent.blue.y == y && gs->opponent.blue.dir == d)
			|| (gs->self.orange.x == x && gs->self.orange.y == y && gs->self.orange.dir == d)
			|| (gs->self.blue.x == x && gs->self.blue.y == y && gs->self.blue.dir == d);
}

portal* other_portal(game_state* gs, unsigned x, unsigned y, direction d) {
	if (gs->opponent.orange.x == x && gs->opponent.orange.y == y && gs->opponent.orange.dir == d) {
		return gs->opponent.blue.x != -1 && gs->opponent.blue.y != -1 ? &gs->opponent.blue : NULL;
	} else if (gs->opponent.blue.x == x && gs->opponent.blue.y == y && gs->opponent.blue.dir == d) {
		return gs->opponent.orange.x != -1 && gs->opponent.orange.y != -1 ? &gs->opponent.orange : NULL;
	} else if (gs->self.orange.x == x && gs->self.orange.y == y && gs->self.orange.dir == d) {
		return gs->self.blue.x != -1 && gs->self.blue.y != -1 ? &gs->self.blue : NULL;
	} else if (gs->self.blue.x == x && gs->self.blue.y == y && gs->self.blue.dir == d) {
		return gs->self.orange.x != -1 && gs->self.orange.y != -1 ? &gs->self.orange : NULL;
	} else {
		return NULL;
	}
}

void place_future_trails(unsigned* thresh, bomb* bomb, unsigned fuse, game_state* gs, unsigned* axis, int dir) {
	unsigned i, true_range = bomb->range, orig_x = bomb->x, orig_y = bomb->y;
	portal* p;
	for (i = 1; i <= true_range; ++i) {
		*axis += dir;
		if (*axis > gs->board_length) {
			break;
		}
		if ((p = other_portal(gs, bomb->x, bomb->y, rev_trail_direction(bomb, axis, dir))) != NULL) {
			bomb->x = p->x;
			bomb->y = p->y;
			direction_to_axis_dir(p->dir, bomb, &axis, &dir);
			*axis += dir;
		}
		if (gs->hard_block_board[xy_to_index(gs,bomb->x,bomb->y)] || gs->soft_block_board[xy_to_index(gs,bomb->x,bomb->y)]) {
			true_range = min(true_range, i + bomb->pierce);
		} else if (thresh[xy_to_index(gs,bomb->x,bomb->y)] > fuse) {
			thresh[xy_to_index(gs,bomb->x,bomb->y)] = fuse;
		}
	}
	bomb->x = orig_x;
	bomb->y = orig_y;
}

void sort_bombs(unsigned* ordering, bomb* bombs, unsigned num_bombs) {
	unsigned i, j, k, min_fuse;
	for (i = 0; i < num_bombs; ++i) {
		min_fuse = -1;
		for (j = 0; j < num_bombs; ++j) {
			if (bombs[j].tick < min_fuse) {
				for (k = 0; k < i; ++k) {
					if (ordering[k] == j) {
						break;
					}
				}
				if (k == i) {
					ordering[i] = j;
					min_fuse = bombs[j].tick;
				}
			}
		}
	}
}

void bomb_thresholds(unsigned* thresh, game_state* gs) {
	unsigned i, *ordered, fuse;
	ordered = (unsigned*) malloc(gs->num_bombs*sizeof(unsigned));
	sort_bombs(ordered, gs->bombs, gs->num_bombs);
	for (i = 0; i < num_cells(gs); ++i) {
		thresh[i] = -1;
	}
	for (i = 0; i < gs->num_bombs; ++i) {
		fuse = min(thresh[xy_to_index(gs, gs->bombs[ordered[i]].x, gs->bombs[ordered[i]].y)], gs->bombs[ordered[i]].tick);
		thresh[xy_to_index(gs, gs->bombs[ordered[i]].x, gs->bombs[ordered[i]].y)] = 0;
		place_future_trails(thresh, &gs->bombs[ordered[i]], fuse, gs, &gs->bombs[ordered[i]].x, 1);
		place_future_trails(thresh, &gs->bombs[ordered[i]], fuse, gs, &gs->bombs[ordered[i]].x, -1);
		place_future_trails(thresh, &gs->bombs[ordered[i]], fuse, gs, &gs->bombs[ordered[i]].y, 1);
		place_future_trails(thresh, &gs->bombs[ordered[i]], fuse, gs, &gs->bombs[ordered[i]].y, -1);
	}
	free(ordered);
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
	if (dists[xy_to_index(gs, dx, dy)] > 1) {
		return NONE;
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
	if (!state->done_mining &&
			(exists_path(gs, NULL, gs->self.x, gs->self.y, gs->board_length/2, gs->board_length/2)
			 || exists_path(gs, NULL, gs->self.x, gs->self.y, gs->opponent.x, gs->opponent.y))) {
		state->done_mining = 1;
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
