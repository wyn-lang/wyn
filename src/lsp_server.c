// Minimal Wyn LSP Server
// Provides basic IDE support
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void send_response(const char* id, const char* result) {
    printf("Content-Length: %zu\r\n\r\n", strlen(result) + 50);
    printf("{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":%s}\n", id, result);
    fflush(stdout);
}

void send_notification(const char* method, const char* params) {
    char msg[4096];
    snprintf(msg, 4096, "{\"jsonrpc\":\"2.0\",\"method\":\"%s\",\"params\":%s}", method, params);
    printf("Content-Length: %zu\r\n\r\n%s\n", strlen(msg), msg);
    fflush(stdout);
}

int main() {
    fprintf(stderr, "Wyn LSP Server starting...\n");
    
    char line[8192];
    while (fgets(line, sizeof(line), stdin)) {
        // Simple LSP message handling
        if (strstr(line, "initialize")) {
            send_response("1", "{\"capabilities\":{\"textDocumentSync\":1}}");
        } else if (strstr(line, "shutdown")) {
            send_response("2", "null");
        } else if (strstr(line, "exit")) {
            break;
        } else if (strstr(line, "textDocument/completion")) {
            // Basic completion
            const char* completions = "{\"items\":[{\"label\":\"print\"},{\"label\":\"import\"},{\"label\":\"fn\"},{\"label\":\"const\"},{\"label\":\"let\"}]}";
            send_response("3", completions);
        }
    }
    
    fprintf(stderr, "Wyn LSP Server stopped\n");
    return 0;
}
