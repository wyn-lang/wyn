// JSON Runtime Wrappers for Wyn
#include "json.h"

// Wrapper functions with Wyn naming convention
WynJson* Json_parse(const char* text) {
    return json_parse(text);
}

char* Json_get_string(WynJson* json, const char* key) {
    return json_get_string(json, key);
}

int Json_get_int(WynJson* json, const char* key) {
    return json_get_int(json, key);
}

void Json_free(WynJson* json) {
    json_free(json);
}
