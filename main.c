
#include <curl/curl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char *argv[]) {
	CURLcode ret;
	CURL *hnd;
	char *resp;
	size_t resp_len;
	json_object *jresp, *gameid, *playerid;
	FILE *fresp = open_memstream(&resp, &resp_len);
	char game_url[sizeof(GAME_URL_TEMPLATE)/sizeof(char)] = GAME_URL_TEMPLATE;
	char post[sizeof(POST_TEMPLATE)/sizeof(char)] = POST_TEMPLATE;
	char* moves[] = {"mu", "ml", "md", "mr", "tu", "tl", "td", "tr", "b", "op", "bp", "", "buy_count", "buy_pierce", "buy_range", "buy_block"};

	hnd = curl_easy_init();
	curl_easy_setopt(hnd, CURLOPT_URL, "http://aicomp.io/api/games/practice");
	curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, START_GAME_POST);
	curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void*) fresp);

	ret = curl_easy_perform(hnd);
	fflush(fresp);
	jresp = json_tokener_parse(resp);
	printf("%s\n", json_object_to_json_string(jresp));
	if (!json_object_object_get_ex(jresp, "gameID", &gameid) || !json_object_object_get_ex(jresp, "playerID", &playerid)) {
		printf("Got bad response from game server!");
	} else {
		strncpy(game_url + GAME_ID_OFFSET, json_object_get_string(gameid), 24);
		strncpy(post + PLAYER_ID_OFFSET, json_object_get_string(playerid), 24);
		printf("%s\n%s\n", json_object_get_string(gameid), game_url);
		printf("%s\n%s\n", json_object_get_string(playerid), post);
		srand((unsigned int) strtoul(json_object_get_string(playerid), NULL, 16));
	    curl_easy_setopt(hnd, CURLOPT_URL, game_url);
		curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, post);
	    while (1) {
			strncpy(post + MOVE_OFFSET, moves[rand() % (sizeof(moves)/sizeof(char*))], 11);
			curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, post);
			printf("%s %d\n", post, curl_easy_perform(hnd));
	    }
	}

	curl_easy_cleanup(hnd);
	json_object_put(jresp);
	fclose(fresp);
	hnd = NULL;

	return (int)ret;
}
