
#include <curl/curl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#include <unistd.h> // for sleep
#endif

#include "ai.h"
#include "game_state.h"

#define GLUE_START(DK,UN) "devkey=" #DK "&username=" #UN
#define XGLUE_START(DK,UN) GLUE_START(DK,UN)
#define GLUE_POST(DK) "devkey=" #DK "&playerID=XXXXXXXXXXXXXXXXXXXXXXXX&move=XXXXXXXXXX"
#define XGLUE_POST(DK) GLUE_POST(DK)

#define GAME_URL_TEMPLATE "http://aicomp.io/api/games/submit/XXXXXXXXXXXXXXXXXXXXXXXX"
#define GAME_ID_OFFSET sizeof(GAME_URL_TEMPLATE) - 25
#define START_GAME_POST XGLUE_START(DEVKEY,USERNAME)
#define POST_TEMPLATE XGLUE_POST(DEVKEY)
#define PLAYER_ID_OFFSET sizeof(POST_TEMPLATE) - 41
#define MOVE_OFFSET sizeof(POST_TEMPLATE) - 11

void extract_portal(json_object* data, portal* portal) {
	json_object* cur;
	if (!json_object_is_type(data, json_type_null)) {
		json_object_object_get_ex(data, "x", &cur);
		portal->x = json_object_get_int(cur);
		json_object_object_get_ex(data, "y", &cur);
		portal->y = json_object_get_int(cur);
		json_object_object_get_ex(data, "direction", &cur);
		portal->dir = json_object_get_int(cur);
	} else {
		portal->x = -1;
		portal->y = -1;
	}
}

void extract_player(json_object* data, player* player) {
	json_object* cur = NULL;
	json_object_object_get_ex(data, "bluePortal", &cur);
	extract_portal(cur, &player->blue);
	cur = NULL;
	json_object_object_get_ex(data, "orangePortal", &cur);
	extract_portal(cur, &player->orange);
	json_object_object_get_ex(data, "orientation", &cur);
	player->orientation = json_object_get_int(cur);
	json_object_object_get_ex(data, "bombRange", &cur);
	player->bomb_range = json_object_get_int(cur);
	json_object_object_get_ex(data, "bombCount", &cur);
	player->bomb_count_cur = json_object_get_int(cur);
	player->bomb_count_max = player->bomb_count_cur;
	json_object_object_get_ex(data, "bombPierce", &cur);
	player->bomb_pierce = json_object_get_int(cur);
	json_object_object_get_ex(data, "coins", &cur);
	player->coins = json_object_get_int(cur);
	json_object_object_get_ex(data, "x", &cur);
	player->x = json_object_get_int(cur);
	json_object_object_get_ex(data, "y", &cur);
	player->y = json_object_get_int(cur);
}

void extract_board(json_object* data, int* board) {
	int i;
	for (i = 0; i < json_object_array_length(data); ++i) {
		board[i] = json_object_get_int(json_object_array_get_idx(data, i));
	}
}

void string_to_pos(const char* str, unsigned* x, unsigned* y) {
	char* copy;
	*x = strtoul(str,&copy,10);
	*y = strtoul(copy+1,NULL,10);
}

game_state* extract_state(json_object* game) {
	game_state* new_state;
	json_object* cur1, *cur2;
	struct json_object_iterator itr, end, itr2, end2;
	int player_index;
	unsigned board_size, num_bombs = 0, num_trails = 0, num;
	if (!json_object_object_get_ex(game, "boardSize", &cur1)) {
		return NULL;
	}
	board_size = json_object_get_int(cur1);
	if (json_object_object_get_ex(game, "bombMap", &cur1)) {
		num_bombs = json_object_object_length(cur1);
	}
	if (json_object_object_get_ex(game, "trailMap", &cur2)) {
		num_trails = json_object_object_length(cur2);
	}
	new_state = game_state_init(board_size, num_bombs, num_trails);
	if (num_trails > 0) {
		itr = json_object_iter_begin(cur2);
		end = json_object_iter_end(cur2);
		while (!json_object_iter_equal(&itr, &end)) {
			--num_trails;
			string_to_pos(json_object_iter_peek_name(&itr), &new_state->trails[num_trails].x, &new_state->trails[num_trails].y);
			new_state->trails[num_trails].tick = 0;
			cur2 = json_object_iter_peek_value(&itr);
			itr2 = json_object_iter_begin(cur2);
			end2 = json_object_iter_end(cur2);
			while (!json_object_iter_equal(&itr2, &end2)) {
				json_object_object_get_ex(json_object_iter_peek_value(&itr2), "tick", &cur2);
				num = json_object_get_int(cur2);
				if (num > new_state->trails[num_trails].tick) {
					new_state->trails[num_trails].tick = num;
				}
				json_object_iter_next(&itr2);
			}
			json_object_iter_next(&itr);
		}
	}
	json_object_object_get_ex(game, "player", &cur2);
	extract_player(cur2, &new_state->self);
	json_object_object_get_ex(game, "opponent", &cur2);
	extract_player(cur2, &new_state->opponent);
	json_object_object_get_ex(game, "hardBlockBoard", &cur2);
	extract_board(cur2, new_state->hard_block_board);
	json_object_object_get_ex(game, "softBlockBoard", &cur2);
	extract_board(cur2, new_state->soft_block_board);
	json_object_object_get_ex(game, "moveIterator", &cur2);
	new_state->turn_start = !json_object_get_int(cur2);
	json_object_object_get_ex(game, "playerIndex", &cur2);
	player_index = json_object_get_int(cur2);
	if (num_bombs > 0) {
		itr = json_object_iter_begin(cur1);
		end = json_object_iter_end(cur1);
		while (!json_object_iter_equal(&itr, &end)) {
			--num_bombs;
			string_to_pos(json_object_iter_peek_name(&itr), &new_state->bombs[num_bombs].x, &new_state->bombs[num_bombs].y);
			cur1 = json_object_iter_peek_value(&itr);
			json_object_object_get_ex(cur1, "tick", &cur2);
			new_state->bombs[num_bombs].tick = json_object_get_int(cur2);
			json_object_object_get_ex(cur1, "owner", &cur2);
			if (json_object_get_int(cur2) == player_index) {
				++new_state->self.bomb_count_max;
				new_state->bombs[num_bombs].range = new_state->self.bomb_range;
				new_state->bombs[num_bombs].pierce = new_state->self.bomb_pierce;
			} else {
				++new_state->opponent.bomb_count_max;
				new_state->bombs[num_bombs].range = new_state->opponent.bomb_range;
				new_state->bombs[num_bombs].pierce = new_state->opponent.bomb_pierce;
			}
			json_object_iter_next(&itr);
		}
	}
	return new_state;
}

int main(int argc, char *argv[]) {
	CURLcode ret;
	CURL *hnd;
	char *resp;
	size_t resp_len;
	json_object *jresp, *gameid, *playerid, *status;
	game_state* gstate;
	FILE *fresp = open_memstream(&resp, &resp_len);
	char game_url[sizeof(GAME_URL_TEMPLATE)/sizeof(char)] = GAME_URL_TEMPLATE;
	char post[sizeof(POST_TEMPLATE)/sizeof(char)] = POST_TEMPLATE;
	char* moves[] = {"mu", "ml", "md", "mr", "tu", "tl", "td", "tr", "b", "op", "bp", "", "buy_count", "buy_pierce", "buy_range", "buy_block"};
	ai_state* state;
	move_enum move;
	int no_err = 0;

#ifdef DEBUG
	printf("initial post: %s\n", START_GAME_POST);
#endif

	hnd = curl_easy_init();
	curl_easy_setopt(hnd, CURLOPT_URL, "http://aicomp.io/api/games/search");
	curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, START_GAME_POST);
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void*) fresp);

	ret = curl_easy_perform(hnd);
	fflush(fresp);
	jresp = json_tokener_parse(resp);
	if (!json_object_object_get_ex(jresp, "gameID", &gameid) || !json_object_object_get_ex(jresp, "playerID", &playerid)) {
		printf("Got bad response from game server!\n");
		printf("%s\n", json_object_to_json_string(jresp));
	} else {
		state = ai_state_init();
		strncpy(game_url + GAME_ID_OFFSET, json_object_get_string(gameid), 24);
		strncpy(post + PLAYER_ID_OFFSET, json_object_get_string(playerid), 24);
#ifdef DEBUG
		printf("GameID:   %s\n", json_object_get_string(gameid));
		printf("Game URL: %s\n", game_url);
		printf("PlayerID: %s\n\n", json_object_get_string(playerid));
#endif
	    curl_easy_setopt(hnd, CURLOPT_URL, game_url);
		curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, post);
	    while (json_object_object_get_ex(jresp, "state", &status) && (no_err = strcmp(json_object_get_string(status), "in progress")) == 0) {
			rewind(fresp);
#ifdef DEBUG
			printf("%s\n", json_object_to_json_string(jresp));
#endif
			gstate = extract_state(jresp);
			move = next_move(state, gstate);
			game_state_dec_ref(gstate);
#ifdef DEBUG
			printf("Move: '%s'\n\n", moves[move]);
#endif
			json_object_put(jresp);
			strncpy(post + MOVE_OFFSET, moves[move], 11);
			curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, post);
			ret = curl_easy_perform(hnd);
			fflush(fresp);
#ifdef DEBUG
			//sleep(1);
#endif
			jresp = json_tokener_parse(resp);
	    }
	}
	ai_state_free(state);
	curl_easy_cleanup(hnd);
	json_object_put(jresp);
	fclose(fresp);
	free(resp);

	return (int)ret;
}
