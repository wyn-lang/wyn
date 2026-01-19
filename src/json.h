#ifndef WYN_JSON_H
#define WYN_JSON_H

typedef struct WynJson WynJson;

WynJson* json_parse(const char* text);
char* json_get_string(WynJson* json, const char* key);
int json_get_int(WynJson* json, const char* key);
void json_free(WynJson* json);

#endif
