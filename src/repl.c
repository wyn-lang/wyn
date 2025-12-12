// Simple Wyn REPL
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Wyn REPL v0.1.0\n");
    printf("Type expressions to evaluate, 'exit' to quit\n");
    printf("\n");
    
    char line[1024];
    int stmt_count = 0;
    
    while (1) {
        printf("wyn> ");
        fflush(stdout);
        
        if (!fgets(line, sizeof(line), stdin)) break;
        
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            break;
        }
        
        if (strlen(line) == 0) continue;
        
        // Create temp file with expression
        char filename[64];
        snprintf(filename, 64, "/tmp/wyn_repl_%d.wyn", stmt_count++);
        
        FILE* f = fopen(filename, "w");
        fprintf(f, "fn main() {\n");
        fprintf(f, "    print(%s)\n", line);
        fprintf(f, "}\n");
        fclose(f);
        
        // Compile and run
        char cmd[256];
        snprintf(cmd, 256, "./build/wync %s -o /tmp/wyn_repl_out 2>/dev/null && /tmp/wyn_repl_out 2>/dev/null", filename);
        system(cmd);
        
        // Cleanup
        unlink(filename);
        unlink("/tmp/wyn_repl_out");
    }
    
    printf("\nGoodbye!\n");
    return 0;
}
