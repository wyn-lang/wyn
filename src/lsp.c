// Minimal but functional LSP server for Wyn
// Implements Language Server Protocol for IDE integration
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Simple JSON-RPC message handling
static void send_response(const char* id, const char* result) {
    printf("Content-Length: %zu\r\n\r\n", strlen(result) + 50);
    printf("{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":%s}\n", id, result);
    fflush(stdout);
}

static void send_notification(const char* method, const char* params) {
    char msg[4096];
    snprintf(msg, sizeof(msg), "{\"jsonrpc\":\"2.0\",\"method\":\"%s\",\"params\":%s}", 
             method, params);
    printf("Content-Length: %zu\r\n\r\n%s\n", strlen(msg), msg);
    fflush(stdout);
}

static char* read_message() {
    char header[256];
    int content_length = 0;
    
    // Read headers
    while (fgets(header, sizeof(header), stdin)) {
        if (strcmp(header, "\r\n") == 0) break;
        if (strncmp(header, "Content-Length:", 15) == 0) {
            content_length = atoi(header + 15);
        }
    }
    
    if (content_length == 0) return NULL;
    
    // Read content
    char* content = malloc(content_length + 1);
    if (fread(content, 1, content_length, stdin) != (size_t)content_length) {
        free(content);
        return NULL;
    }
    content[content_length] = '\0';
    
    return content;
}

static void handle_initialize(const char* id) {
    const char* capabilities = 
        "{\"capabilities\":{"
        "\"textDocumentSync\":1,"
        "\"hoverProvider\":true,"
        "\"definitionProvider\":true,"
        "\"completionProvider\":{\"triggerCharacters\":[\".\"]}"
        "}}";
    send_response(id, capabilities);
}

static void handle_hover(const char* id) {
    // Simple hover response
    const char* hover = "{\"contents\":\"Wyn type information\"}";
    send_response(id, hover);
}

static void handle_completion(const char* id) {
    // Simple completion response
    const char* completions = 
        "[{\"label\":\"print\",\"kind\":3},"
        "{\"label\":\"var\",\"kind\":14},"
        "{\"label\":\"fn\",\"kind\":14}]";
    send_response(id, completions);
}

int lsp_server_start() {
    fprintf(stderr, "Wyn Language Server starting...\n");
    fprintf(stderr, "LSP Protocol: JSON-RPC 2.0\n");
    fprintf(stderr, "Listening on stdin/stdout...\n");
    
    while (1) {
        char* msg = read_message();
        if (!msg) break;
        
        // Parse method and id (simple parsing)
        char* method = strstr(msg, "\"method\":");
        char* id_str = strstr(msg, "\"id\":");
        
        if (!method) {
            free(msg);
            continue;
        }
        
        method += 9;
        while (*method && (*method == ' ' || *method == '"')) method++;
        
        char id[64] = "null";
        if (id_str) {
            id_str += 5;
            while (*id_str && (*id_str == ' ' || *id_str == ':')) id_str++;
            int i = 0;
            while (*id_str && *id_str != ',' && *id_str != '}' && i < 63) {
                id[i++] = *id_str++;
            }
            id[i] = '\0';
        }
        
        // Handle methods
        if (strncmp(method, "initialize", 10) == 0) {
            handle_initialize(id);
        } else if (strncmp(method, "initialized", 11) == 0) {
            // No response needed
        } else if (strncmp(method, "shutdown", 8) == 0) {
            send_response(id, "null");
            free(msg);
            break;
        } else if (strncmp(method, "textDocument/hover", 18) == 0) {
            handle_hover(id);
        } else if (strncmp(method, "textDocument/completion", 23) == 0) {
            handle_completion(id);
        } else if (strncmp(method, "textDocument/definition", 23) == 0) {
            send_response(id, "null");
        } else {
            // Unknown method - send null response
            if (strcmp(id, "null") != 0) {
                send_response(id, "null");
            }
        }
        
        free(msg);
    }
    
    fprintf(stderr, "LSP server stopped\n");
    return 0;
}
