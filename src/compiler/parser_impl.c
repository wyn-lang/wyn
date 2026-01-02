            lexer_advance(l);
            if (lexer_char(l) == '?') { lexer_advance(l); t.kind = TOK_QUESTIONQUESTION; }
            else if (lexer_char(l) == '.') { lexer_advance(l); t.kind = TOK_QUESTIONDOT; }
            else { t.kind = TOK_QUESTION; }
            break;
        case '@': lexer_advance(l); t.kind = TOK_AT; break;
        case '(': lexer_advance(l); t.kind = TOK_LPAREN; break;
        case ')': lexer_advance(l); t.kind = TOK_RPAREN; break;
        case '{': lexer_advance(l); t.kind = TOK_LBRACE; break;
        case '}': lexer_advance(l); t.kind = TOK_RBRACE; break;
        case '[': lexer_advance(l); t.kind = TOK_LBRACKET; break;
        case ']': lexer_advance(l); t.kind = TOK_RBRACKET; break;
        case ',': lexer_advance(l); t.kind = TOK_COMMA; break;
        case ':': lexer_advance(l); t.kind = TOK_COLON; break;
        case ';': lexer_advance(l); t.kind = TOK_SEMICOLON; break;
        default:
            lexer_advance(l);
            t.kind = TOK_ERROR;
            break;
    }
    
    l->current = t;
    return t;
}

Token lexer_peek(Lexer* l) {
    if (l->has_peek) {
        return l->peek;
    }
    Token current = l->current;
    l->peek = lexer_next(l);
    l->current = current;
    l->has_peek = true;
    return l->peek;
}

// ============================================================================
// SECTION 2: PARSER - AST Generation
// Lines 625-2922 (~2300 lines)
// 
// Builds Abstract Syntax Tree from tokens.
// Key functions: parse_module(), parse_expr(), parse_stmt()
// ============================================================================

// ============================================================================
// AST Node Types
// ============================================================================

typedef enum {
    TYPE_INT, TYPE_FLOAT, TYPE_STR, TYPE_BOOL, TYPE_BYTE, TYPE_ANY, TYPE_VOID,
    TYPE_ARRAY, TYPE_MAP, TYPE_TUPLE, TYPE_OPTIONAL, TYPE_RESULT,
    TYPE_FUNCTION, TYPE_NAMED, TYPE_GENERIC, TYPE_POINTER
} TypeKind;

struct Type {
    TypeKind kind;
    char name[MAX_IDENT_LEN];
    Type* inner;          // for array, optional
    Type* key_type;       // for map
    Type* value_type;     // for map, result
    Type** params;        // for tuple, function params
    int param_count;
    Type* return_type;    // for function
};

typedef enum {
    EXPR_INT, EXPR_FLOAT, EXPR_STRING, EXPR_BOOL, EXPR_IDENT,
    EXPR_BINARY, EXPR_UNARY, EXPR_CALL, EXPR_INDEX, EXPR_FIELD,
    EXPR_ARRAY, EXPR_MAP, EXPR_TUPLE, EXPR_LAMBDA, EXPR_STRUCT,
    EXPR_IF, EXPR_MATCH, EXPR_SOME, EXPR_NONE, EXPR_OK, EXPR_ERR,
    EXPR_UNWRAP, EXPR_TRY, EXPR_DEFAULT, EXPR_SLICE, EXPR_ADDR, EXPR_DEREF, EXPR_AWAIT, EXPR_TERNARY, EXPR_COMPREHENSION
} ExprKind;

struct Expr {
    ExprKind kind;
    int line, column;
    union {
        int64_t int_val;
        double float_val;
        char str_val[MAX_STRING_LEN];
        bool bool_val;
        char ident[MAX_IDENT_LEN];
        struct { Expr* left; Expr* right; TokenKind op; } binary;
        struct { Expr* operand; TokenKind op; } unary;
        struct { Expr* func; Expr** args; int arg_count; Type** type_args; int type_arg_count; } call;
        struct { Expr* object; Expr* index; bool safe; } index;
        struct { Expr* object; char field[MAX_IDENT_LEN]; } field;
        struct { Expr** elements; int count; bool is_empty_literal; } array;
        struct { char** keys; Expr** values; int count; } map;
        struct { char name[MAX_IDENT_LEN]; char fields[MAX_FIELDS][MAX_IDENT_LEN]; Expr* values[MAX_FIELDS]; int count; Type** type_args; int type_arg_count; } struct_lit;
        struct { Expr* cond; Expr* then_expr; Expr* else_expr; } if_expr;
        struct { Expr* value; Expr** patterns; Expr** arms; int arm_count; char bindings[64][MAX_IDENT_LEN]; Type** binding_types; } match_expr;
        struct { Expr* value; } some;  // also used for ok/err
        struct { Expr* value; Expr* default_val; } default_expr;
        struct { Expr* object; Expr* start; Expr* end; Expr* step; } slice;
        struct { char params[8][MAX_IDENT_LEN]; Type* param_types[8]; int param_count; Type* return_type; Stmt** body; int body_count; int id; } lambda;
        struct { Expr* future; } await_expr;
        struct { Expr* cond; Expr* true_val; Expr* false_val; } ternary;
        struct { Expr* expr; char var[MAX_IDENT_LEN]; Expr* iter; Expr* condition; } comprehension;
    };
};

typedef enum {
    STMT_LET, STMT_ASSIGN, STMT_EXPR, STMT_RETURN, STMT_IF, STMT_WHILE,
    STMT_FOR, STMT_MATCH, STMT_BLOCK, STMT_BREAK, STMT_CONTINUE, STMT_DEFER, STMT_SPAWN
} StmtKind;

struct Stmt {
    StmtKind kind;
    int line, column;
    union {
        struct { char name[MAX_IDENT_LEN]; Type* type; Expr* value; bool is_mut; bool is_pub; } let;
        struct { Expr* target; Expr* value; TokenKind op; } assign;
        struct { Expr* expr; } expr;
        struct { Expr* value; } ret;
        struct { Expr* cond; Stmt** then_block; int then_count; Stmt** else_block; int else_count; } if_stmt;
        struct { Expr* cond; Stmt** body; int body_count; } while_stmt;
        struct { char var[MAX_IDENT_LEN]; Expr* iter; Stmt** body; int body_count; bool is_parallel; } for_stmt;
        struct { Expr* value; Expr** patterns; Stmt*** arms; int* arm_counts; int arm_count; char bindings[64][MAX_IDENT_LEN]; Type** binding_types; } match_stmt;
        struct { Stmt** stmts; int count; } block;
        struct { Expr* expr; } defer;
        struct { Stmt** body; int body_count; char** captured_vars; int captured_count; Type** captured_types; } spawn;
    };
};

typedef struct {
    char name[MAX_IDENT_LEN];
    Type* type;
    Expr* default_val;
    bool is_variadic;
} Param;

struct FnDef {
    char name[MAX_IDENT_LEN];
    Param params[MAX_PARAMS];
    int param_count;
    Type* return_type;
    Stmt** body;
    int body_count;
    bool is_pub;
    bool is_async;
    char generics[MAX_GENERICS][MAX_IDENT_LEN];
    int generic_count;
    bool is_extension;
    char extension_type[MAX_IDENT_LEN];
    char extension_method[MAX_IDENT_LEN];
};

typedef struct {
    char name[MAX_IDENT_LEN];
    Type* type;
    bool is_pub;
} Field;

struct StructDef {
    char name[MAX_IDENT_LEN];
    Field fields[MAX_FIELDS];
    int field_count;
    FnDef* methods;
    int method_count;
    bool is_pub;
    char generics[MAX_GENERICS][MAX_IDENT_LEN];
    int generic_count;
};

typedef struct {
    char name[MAX_IDENT_LEN];
    Type** types;
    int type_count;
    int64_t value;
    bool has_value;
} Variant;

struct EnumDef {
    char name[MAX_IDENT_LEN];
    Variant variants[MAX_VARIANTS];
    int variant_count;
    bool is_pub;
    char generics[MAX_GENERICS][MAX_IDENT_LEN];
    int generic_count;
};

typedef struct {
    char name[MAX_IDENT_LEN];
    FnDef* methods;
    int method_count;
    bool is_pub;
} InterfaceDef;

typedef struct {
    char interface_name[MAX_IDENT_LEN];
    char type_name[MAX_IDENT_LEN];
    FnDef* methods;
    int method_count;
} ImplDef;

typedef struct {
    char path[MAX_STRING_LEN];
    char alias[MAX_IDENT_LEN];
    char** imports;
    int import_count;
} Import;

typedef struct {
    char name[MAX_IDENT_LEN];
    Stmt** body;
    int body_count;
} TestDef;

struct Module {
    char name[MAX_IDENT_LEN];
    Import* imports;
    int import_count;
    FnDef* functions;
    int function_count;
    StructDef* structs;
    int struct_count;
    EnumDef* enums;
    int enum_count;
    InterfaceDef* interfaces;
    int interface_count;
    ImplDef* impls;
    int impl_count;
    TestDef* tests;
    int test_count;
    Stmt** top_level_stmts;
    int top_level_count;
};

// ============================================================================
// Parser
// ============================================================================

typedef struct {
    Lexer* lexer;
    Token current;
    const char* filename;
    bool had_error;
    int lambda_count;
    bool newline_before;  // True if newline was skipped before current token
} Parser;

static Parser* parser_new(Lexer* lexer, const char* filename) {
    Parser* p = malloc(sizeof(Parser));
    p->lexer = lexer;
    p->filename = filename;
    p->had_error = false;
    p->lambda_count = 0;
    p->newline_before = false;
    p->current = lexer_next(lexer);
    return p;
}

static void parser_free(Parser* p) { free(p); }

// Helper: Map module.function() to builtin function name
// Returns the builtin name if it's a module call, NULL otherwise
static const char* map_module_function(const char* module, const char* function) {
    // Builtin mappings - but type checker enforces imports are required
    if (strcmp(module, "string") == 0) {
        if (strcmp(function, "split") == 0) return "str_split";
        if (strcmp(function, "find") == 0) return "str_find";
        if (strcmp(function, "concat") == 0) return "str_concat";
        if (strcmp(function, "cmp") == 0) return "str_cmp";
        if (strcmp(function, "substring") == 0) return "substring";
        if (strcmp(function, "char_at") == 0) return "char_at";
        if (strcmp(function, "chr") == 0) return "chr";
        if (strcmp(function, "ord") == 0) return "ord";
        if (strcmp(function, "int_to_str") == 0) return "int_to_str";
        if (strcmp(function, "from_int") == 0) return "int_to_str";
        if (strcmp(function, "upper") == 0) return "str_upper";
        if (strcmp(function, "lower") == 0) return "str_lower";
        if (strcmp(function, "trim") == 0) return "str_trim";
    } else if (strcmp(module, "compress") == 0) {
        if (strcmp(function, "gzip_file") == 0) return "gzip_file";
        if (strcmp(function, "gunzip_file") == 0) return "gunzip_file";
        if (strcmp(function, "tar_create") == 0) return "tar_create";
        if (strcmp(function, "tar_extract") == 0) return "tar_extract";
        if (strcmp(function, "tar_list") == 0) return "tar_list";
        if (strcmp(function, "compress_string") == 0) return "compress_string";
        if (strcmp(function, "decompress_string") == 0) return "decompress_string";
    } else if (strcmp(module, "regex") == 0) {
        if (strcmp(function, "matches") == 0) return "regex_matches";
        if (strcmp(function, "find") == 0) return "regex_find";
        if (strcmp(function, "replace") == 0) return "regex_replace";
        if (strcmp(function, "count") == 0) return "regex_count";
    } else if (strcmp(module, "time") == 0) {
        if (strcmp(function, "now_unix") == 0) return "now_unix";
        if (strcmp(function, "now_millis") == 0) return "now_millis";
        if (strcmp(function, "now_micros") == 0) return "now_micros";
        if (strcmp(function, "sleep") == 0) return "sleep_seconds";
        if (strcmp(function, "format_timestamp") == 0) return "format_timestamp";
        if (strcmp(function, "format_iso8601") == 0) return "format_iso8601";
        if (strcmp(function, "parse_timestamp") == 0) return "parse_timestamp";
        // Legacy mappings
        if (strcmp(function, "sleep_ms") == 0) return "sleep_ms";
        if (strcmp(function, "now") == 0) return "time_now";
        if (strcmp(function, "clock") == 0) return "clock";
    } else if (strcmp(module, "os") == 0) {
        if (strcmp(function, "getenv") == 0) return "getenv_wyn";
        if (strcmp(function, "setenv") == 0) return "setenv_wyn";
        if (strcmp(function, "exec") == 0) return "exec_wyn";
        if (strcmp(function, "exec_output") == 0) return "exec_output_wyn";
        if (strcmp(function, "exec_args") == 0) return "exec_args_wyn";
        if (strcmp(function, "exec_capture") == 0) return "os_exec_capture";
        if (strcmp(function, "exec_check") == 0) return "os_exec_check";
        if (strcmp(function, "exit") == 0) return "exit_wyn";
        if (strcmp(function, "args") == 0) return "args_wyn";
        if (strcmp(function, "cwd") == 0) return "cwd_wyn";
        if (strcmp(function, "chdir") == 0) return "chdir_wyn";
        if (strcmp(function, "hostname") == 0) return "hostname_wyn";
        if (strcmp(function, "getpid") == 0) return "getpid_wyn";
        // Command builder
        if (strcmp(function, "command_start") == 0) return "command_start";
        if (strcmp(function, "command_arg") == 0) return "command_arg";
        if (strcmp(function, "command_run") == 0) return "command_run";
        if (strcmp(function, "command_output") == 0) return "command_output";
        // New chaining API
        if (strcmp(function, "command") == 0) return "command_new";
        if (strcmp(function, "arg") == 0) return "command_arg_chain";
        if (strcmp(function, "run") == 0) return "command_run_chain";
        if (strcmp(function, "output") == 0) return "command_output_chain";
        // Legacy mappings
        if (strcmp(function, "getpid") == 0) return "getpid";
        if (strcmp(function, "getcwd") == 0) return "getcwd";
        if (strcmp(function, "system") == 0) return "system";
    } else if (strcmp(module, "json") == 0) {
        if (strcmp(function, "parse") == 0) return "parse_json";
        if (strcmp(function, "stringify") == 0) return "stringify_json";
        if (strcmp(function, "get_string") == 0) return "get_string_json";
        if (strcmp(function, "get_int") == 0) return "get_int_json";
        if (strcmp(function, "get_bool") == 0) return "get_bool_json";
    } else if (strcmp(module, "config") == 0) {
        if (strcmp(function, "yaml_parse") == 0) return "yaml_parse";
        if (strcmp(function, "toml_parse") == 0) return "toml_parse";
    } else if (strcmp(module, "net") == 0) {
        if (strcmp(function, "http_get") == 0) return "http_get_wyn";
        if (strcmp(function, "http_post") == 0) return "http_post_wyn";
        if (strcmp(function, "tcp_connect") == 0) return "tcp_connect_wyn";
        if (strcmp(function, "tcp_send") == 0) return "tcp_send_wyn";
        if (strcmp(function, "tcp_recv") == 0) return "tcp_recv_wyn";
        if (strcmp(function, "tcp_close") == 0) return "tcp_close_wyn";
        if (strcmp(function, "http_put") == 0) return "http_put_wyn";
        if (strcmp(function, "http_delete") == 0) return "http_delete_wyn";
        if (strcmp(function, "http_head") == 0) return "http_head_wyn";
        if (strcmp(function, "http_get_headers") == 0) return "http_get_headers_wyn";
        if (strcmp(function, "udp_send") == 0) return "udp_send_wyn";
        if (strcmp(function, "udp_recv") == 0) return "udp_recv_wyn";
        if (strcmp(function, "resolve_host") == 0) return "resolve_host_wyn";
        if (strcmp(function, "get_local_ip") == 0) return "get_local_ip_wyn";
        if (strcmp(function, "server_create") == 0) return "http_server_create";
        if (strcmp(function, "server_accept") == 0) return "http_server_accept";
        if (strcmp(function, "server_read_request") == 0) return "http_server_read_request";
        if (strcmp(function, "parse_method") == 0) return "http_parse_method";
        if (strcmp(function, "parse_path") == 0) return "http_parse_path";
        if (strcmp(function, "parse_body") == 0) return "http_parse_body";
        if (strcmp(function, "server_send_response") == 0) return "http_server_send_response";
        if (strcmp(function, "server_send_json") == 0) return "http_server_send_json";
        if (strcmp(function, "server_close_client") == 0) return "http_server_close_client";
        if (strcmp(function, "server_close") == 0) return "http_server_close";
        if (strcmp(function, "args") == 0) return "args";
        if (strcmp(function, "args") == 0) return "args";
    } else if (strcmp(module, "io") == 0) {
        // Console I/O only
        if (strcmp(function, "print") == 0) return "print";
        if (strcmp(function, "println") == 0) return "println";
        if (strcmp(function, "print_str") == 0) return "print_str";
        if (strcmp(function, "print_int") == 0) return "print_int";
        if (strcmp(function, "print_float") == 0) return "print_float";
        if (strcmp(function, "print_newline") == 0) return "print_newline";
        if (strcmp(function, "read_line") == 0) return "read_line";
        if (strcmp(function, "input") == 0) return "input";
    } else if (strcmp(module, "fs") == 0) {
        // File operations
        if (strcmp(function, "read") == 0) return "read_file";
        if (strcmp(function, "write") == 0) return "fs_write";
        if (strcmp(function, "append") == 0) return "fs_append";
        if (strcmp(function, "delete") == 0) return "fs_delete";
        if (strcmp(function, "exists") == 0) return "file_exists";
        if (strcmp(function, "size") == 0) return "file_size";
        if (strcmp(function, "copy") == 0) return "file_copy";
        if (strcmp(function, "move") == 0) return "file_move";
        if (strcmp(function, "chmod") == 0) return "file_chmod";
        if (strcmp(function, "glob") == 0) return "file_glob";
        // Directory operations
        if (strcmp(function, "mkdir") == 0) return "fs_mkdir";
        if (strcmp(function, "rmdir") == 0) return "rmdir_wyn";
        if (strcmp(function, "list_dir") == 0) return "list_dir";
        if (strcmp(function, "is_dir") == 0) return "is_dir_wyn";
        if (strcmp(function, "is_file") == 0) return "is_file_wyn";
        // Legacy names for compatibility
        if (strcmp(function, "read_file") == 0) return "read_file";
        if (strcmp(function, "write_file") == 0) return "fs_write";
        if (strcmp(function, "append_file") == 0) return "fs_append";
        if (strcmp(function, "delete_file") == 0) return "fs_delete";
        if (strcmp(function, "file_exists") == 0) return "file_exists";
        if (strcmp(function, "file_size") == 0) return "file_size";
    } else if (strcmp(module, "fs_old") == 0) {
        if (strcmp(function, "read_file") == 0) return "read_file";
        if (strcmp(function, "delete") == 0) return "delete_file";
        if (strcmp(function, "size") == 0) return "file_size";
        if (strcmp(function, "mkdir") == 0) return "mkdir_wyn";
        if (strcmp(function, "rmdir") == 0) return "rmdir_wyn";
        if (strcmp(function, "is_dir") == 0) return "is_dir_wyn";
        if (strcmp(function, "is_file") == 0) return "is_file_wyn";
        if (strcmp(function, "write_file") == 0) return "write_file";
        if (strcmp(function, "append_file") == 0) return "append_file";
        if (strcmp(function, "delete_file") == 0) return "delete_file";
        if (strcmp(function, "file_exists") == 0) return "file_exists";
        if (strcmp(function, "file_size") == 0) return "file_size";
        if (strcmp(function, "is_dir") == 0) return "is_dir";
        if (strcmp(function, "list_dir") == 0) return "list_dir";
        if (strcmp(function, "mkdir") == 0) return "mkdir";
        if (strcmp(function, "rmdir") == 0) return "rmdir";
    } else if (strcmp(module, "math") == 0) {
        if (strcmp(function, "abs") == 0) return "abs";
        if (strcmp(function, "min") == 0) return "min";
        if (strcmp(function, "max") == 0) return "max";
        if (strcmp(function, "pow") == 0) return "pow";
        if (strcmp(function, "sqrt") == 0) return "sqrt";
        if (strcmp(function, "gcd") == 0) return "gcd";
        if (strcmp(function, "lcm") == 0) return "lcm";
        if (strcmp(function, "factorial") == 0) return "factorial";
        if (strcmp(function, "sin") == 0) return "sinf";
        if (strcmp(function, "cos") == 0) return "cosf";
        if (strcmp(function, "tan") == 0) return "tanf";
        if (strcmp(function, "asin") == 0) return "asinf";
        if (strcmp(function, "acos") == 0) return "acosf";
        if (strcmp(function, "atan") == 0) return "atanf";
        if (strcmp(function, "atan2") == 0) return "atan2f";
        if (strcmp(function, "floor") == 0) return "floorf";
        if (strcmp(function, "ceil") == 0) return "ceilf";
        if (strcmp(function, "round") == 0) return "roundf";
        if (strcmp(function, "log") == 0) return "logf";
        if (strcmp(function, "log10") == 0) return "log10f";
        if (strcmp(function, "exp") == 0) return "expf";
    } else if (strcmp(module, "test") == 0) {
        if (strcmp(function, "assert") == 0) return "assert";
    } else if (strcmp(module, "gui") == 0) {
        if (strcmp(function, "create_window") == 0) return "gui_create_window";
        if (strcmp(function, "show_window") == 0) return "gui_show_window";
        if (strcmp(function, "set_title") == 0) return "gui_set_title";
        if (strcmp(function, "run") == 0) return "gui_run";
        if (strcmp(function, "poll_events") == 0) return "gui_poll_events";
        if (strcmp(function, "close_window") == 0) return "gui_close_window";
        if (strcmp(function, "draw_rect") == 0) return "gui_draw_rect";
        if (strcmp(function, "draw_text") == 0) return "gui_draw_text";
    } else if (strcmp(module, "mobile") == 0) {
        if (strcmp(function, "init") == 0) return "mobile_init";
        if (strcmp(function, "run") == 0) return "mobile_run";
        if (strcmp(function, "create_view") == 0) return "mobile_create_view";
        if (strcmp(function, "add_subview") == 0) return "mobile_add_subview";
        if (strcmp(function, "add_to_root") == 0) return "mobile_add_to_root";
        if (strcmp(function, "create_button") == 0) return "mobile_create_button";
        if (strcmp(function, "create_label") == 0) return "mobile_create_label";
        if (strcmp(function, "create_textfield") == 0) return "mobile_create_textfield";
        if (strcmp(function, "set_text") == 0) return "mobile_set_text";
        if (strcmp(function, "get_text") == 0) return "mobile_get_text";
        if (strcmp(function, "set_frame") == 0) return "mobile_set_frame";
        if (strcmp(function, "set_background_color") == 0) return "mobile_set_background_color";
        if (strcmp(function, "set_text_color") == 0) return "mobile_set_text_color";
        if (strcmp(function, "set_font_size") == 0) return "mobile_set_font_size";
        if (strcmp(function, "on_tap") == 0) return "mobile_on_tap";
        if (strcmp(function, "on_text_change") == 0) return "mobile_on_text_change";
        if (strcmp(function, "show_alert") == 0) return "mobile_show_alert";
        if (strcmp(function, "hide_view") == 0) return "mobile_hide_view";
        if (strcmp(function, "show_view") == 0) return "mobile_show_view";
        if (strcmp(function, "remove_view") == 0) return "mobile_remove_view";
    } else if (strcmp(module, "gpu") == 0) {
        if (strcmp(function, "device_count") == 0) return "__wyn_gpu_device_count";
        if (strcmp(function, "device_name") == 0) return "__wyn_gpu_device_name";
        if (strcmp(function, "malloc") == 0) return "__wyn_gpu_malloc";
        if (strcmp(function, "free") == 0) return "__wyn_gpu_free";
        if (strcmp(function, "sync") == 0) return "__wyn_gpu_sync";
    } else if (strcmp(module, "os") == 0) {
        if (strcmp(function, "getenv") == 0) return "getenv_wyn";
        if (strcmp(function, "setenv") == 0) return "setenv_wyn";
        if (strcmp(function, "exec") == 0) return "exec_wyn";
        if (strcmp(function, "exit") == 0) return "exit_wyn";
        if (strcmp(function, "args") == 0) return "args_wyn";
        if (strcmp(function, "cwd") == 0) return "cwd_wyn";
        if (strcmp(function, "chdir") == 0) return "chdir_wyn";
        if (strcmp(function, "hostname") == 0) return "hostname_wyn";
        if (strcmp(function, "getpid") == 0) return "getpid_wyn";
    } else if (strcmp(module, "path") == 0) {
        if (strcmp(function, "join") == 0) return "path_join";
        if (strcmp(function, "dirname") == 0) return "path_dirname";
        if (strcmp(function, "basename") == 0) return "path_basename";
        if (strcmp(function, "ext") == 0) return "path_ext";
    } else if (strcmp(module, "json") == 0) {
        if (strcmp(function, "parse") == 0) return "parse_json";
        if (strcmp(function, "stringify") == 0) return "stringify_json";
        if (strcmp(function, "get_string") == 0) return "get_string_json";
        if (strcmp(function, "get_int") == 0) return "get_int_json";
        if (strcmp(function, "get_bool") == 0) return "get_bool_json";
    } else if (strcmp(module, "config") == 0) {
        if (strcmp(function, "yaml_parse") == 0) return "yaml_parse";
        if (strcmp(function, "toml_parse") == 0) return "toml_parse";
    } else if (strcmp(module, "net") == 0) {
        if (strcmp(function, "http_get") == 0) return "http_get_wyn";
        if (strcmp(function, "http_post") == 0) return "http_post_wyn";
        if (strcmp(function, "tcp_connect") == 0) return "tcp_connect_wyn";
        if (strcmp(function, "tcp_send") == 0) return "tcp_send_wyn";
        if (strcmp(function, "tcp_recv") == 0) return "tcp_recv_wyn";
        if (strcmp(function, "tcp_close") == 0) return "tcp_close_wyn";
        if (strcmp(function, "http_put") == 0) return "http_put_wyn";
        if (strcmp(function, "http_delete") == 0) return "http_delete_wyn";
        if (strcmp(function, "http_head") == 0) return "http_head_wyn";
        if (strcmp(function, "http_get_headers") == 0) return "http_get_headers_wyn";
        if (strcmp(function, "udp_send") == 0) return "udp_send_wyn";
        if (strcmp(function, "udp_recv") == 0) return "udp_recv_wyn";
        if (strcmp(function, "resolve_host") == 0) return "resolve_host_wyn";
        if (strcmp(function, "get_local_ip") == 0) return "get_local_ip_wyn";
        if (strcmp(function, "server_create") == 0) return "http_server_create";
        if (strcmp(function, "server_accept") == 0) return "http_server_accept";
        if (strcmp(function, "server_read_request") == 0) return "http_server_read_request";
        if (strcmp(function, "parse_method") == 0) return "http_parse_method";
        if (strcmp(function, "parse_path") == 0) return "http_parse_path";
        if (strcmp(function, "parse_body") == 0) return "http_parse_body";
        if (strcmp(function, "server_send_response") == 0) return "http_server_send_response";
        if (strcmp(function, "server_send_json") == 0) return "http_server_send_json";
        if (strcmp(function, "server_close_client") == 0) return "http_server_close_client";
        if (strcmp(function, "server_close") == 0) return "http_server_close";
    } else if (strcmp(module, "log") == 0) {
        if (strcmp(function, "info") == 0) return "log_info";
        if (strcmp(function, "warn") == 0) return "log_warn";
        if (strcmp(function, "error") == 0) return "log_error";
        if (strcmp(function, "debug") == 0) return "log_debug";
        if (strcmp(function, "log_with_level") == 0) return "log_with_level";
    } else if (strcmp(module, "encoding") == 0) {
        if (strcmp(function, "base64_encode") == 0) return "base64_encode";
        if (strcmp(function, "base64_decode") == 0) return "base64_decode";
        if (strcmp(function, "hex_encode") == 0) return "hex_encode";
        if (strcmp(function, "hex_decode") == 0) return "hex_decode";
        if (strcmp(function, "url_encode") == 0) return "url_encode";
        if (strcmp(function, "url_decode") == 0) return "url_decode";
    } else if (strcmp(module, "hash") == 0) {
        if (strcmp(function, "sha256") == 0) return "sha256_hash";
        if (strcmp(function, "md5") == 0) return "md5_hash";
        if (strcmp(function, "sha1") == 0) return "sha1_hash";
    } else if (strcmp(module, "hashmap") == 0) {
        if (strcmp(function, "new") == 0) return "hashmap_new";
        if (strcmp(function, "set") == 0) return "hashmap_put";
        if (strcmp(function, "get") == 0) return "hashmap_get";
        if (strcmp(function, "has") == 0) return "hashmap_contains";
        if (strcmp(function, "delete") == 0) return "hashmap_remove";
        if (strcmp(function, "keys") == 0) return "hashmap_keys";
        if (strcmp(function, "values") == 0) return "hashmap_values";
        if (strcmp(function, "size") == 0) return "hashmap_size";
        if (strcmp(function, "clear") == 0) return "hashmap_clear";
    } else if (strcmp(module, "collections") == 0) {
        if (strcmp(function, "hashmap_new") == 0) return "hashmap_new";
        if (strcmp(function, "hashmap_put") == 0) return "hashmap_put";
        if (strcmp(function, "hashmap_get") == 0) return "hashmap_get";
        if (strcmp(function, "hashmap_contains") == 0) return "hashmap_contains";
        if (strcmp(function, "hashmap_remove") == 0) return "hashmap_remove";
        if (strcmp(function, "hashmap_size") == 0) return "hashmap_size";
        if (strcmp(function, "hashmap_clear") == 0) return "hashmap_clear";
        if (strcmp(function, "set_new") == 0) return "set_new";
        if (strcmp(function, "set_add") == 0) return "set_add";
        if (strcmp(function, "set_contains") == 0) return "set_contains";
        if (strcmp(function, "set_remove") == 0) return "set_remove";
        if (strcmp(function, "set_size") == 0) return "set_size";
        if (strcmp(function, "set_clear") == 0) return "set_clear";
    } else if (strcmp(module, "crypto") == 0) {
        if (strcmp(function, "encrypt_aes256") == 0) return "encrypt_aes256";
        if (strcmp(function, "decrypt_aes256") == 0) return "decrypt_aes256";
        if (strcmp(function, "generate_random_bytes") == 0) return "generate_random_bytes";
        if (strcmp(function, "hmac_sha256") == 0) return "hmac_sha256";
        if (strcmp(function, "verify_signature") == 0) return "verify_signature";
    } else if (strcmp(module, "cli") == 0) {
        if (strcmp(function, "has_flag") == 0) return "cli_has_flag";
        if (strcmp(function, "get_arg") == 0) return "cli_get_arg";
        if (strcmp(function, "get_positional") == 0) return "cli_get_positional";
        if (strcmp(function, "arg_count") == 0) return "cli_arg_count";
    } else if (strcmp(module, "array") == 0) {
        if (strcmp(function, "reverse") == 0) return "array_reverse";
        if (strcmp(function, "append") == 0) return "array_append_elem";
        if (strcmp(function, "prepend") == 0) return "array_prepend_elem";
        if (strcmp(function, "contains") == 0) return "array_contains_elem";
        if (strcmp(function, "filter") == 0) return "array_filter";
        if (strcmp(function, "map") == 0) return "array_map";
        if (strcmp(function, "reduce") == 0) return "array_reduce";
    } else if (strcmp(module, "database") == 0) {
        if (strcmp(function, "sqlite_open") == 0) return "sqlite_open";
        if (strcmp(function, "sqlite_exec") == 0) return "sqlite_exec";
        if (strcmp(function, "sqlite_query") == 0) return "sqlite_query";
        if (strcmp(function, "sqlite_close") == 0) return "sqlite_close";
    } else if (strcmp(module, "http") == 0) {
        if (strcmp(function, "router_new") == 0) return "http_router_new";
        if (strcmp(function, "router_add") == 0) return "http_router_add";
        if (strcmp(function, "router_match") == 0) return "http_router_match";
        if (strcmp(function, "response_json") == 0) return "http_response_json";
        if (strcmp(function, "response_html") == 0) return "http_response_html";
        if (strcmp(function, "response_text") == 0) return "http_response_text";
    } else if (strcmp(module, "testing") == 0) {
        if (strcmp(function, "assert_eq") == 0) return "test_assert_eq";
        if (strcmp(function, "assert_ne") == 0) return "test_assert_ne";
        if (strcmp(function, "assert_true") == 0) return "test_assert_true";
        if (strcmp(function, "assert_false") == 0) return "test_assert_false";
        if (strcmp(function, "summary") == 0) return "test_summary";
    } else if (strcmp(module, "result") == 0) {
        if (strcmp(function, "make_ok") == 0) return "result_ok";
        if (strcmp(function, "make_err") == 0) return "result_err";
        if (strcmp(function, "is_ok") == 0) return "result_is_ok";
        if (strcmp(function, "is_err") == 0) return "result_is_err";
        if (strcmp(function, "unwrap") == 0) return "result_unwrap";
        if (strcmp(function, "make_some") == 0) return "option_some";
        if (strcmp(function, "make_none") == 0) return "option_none";
        if (strcmp(function, "is_some") == 0) return "option_is_some";
        if (strcmp(function, "is_none") == 0) return "option_is_none";
        if (strcmp(function, "unwrap_option") == 0) return "option_unwrap";
        if (strcmp(function, "generate_random_bytes") == 0) return "generate_random_bytes";
        if (strcmp(function, "hmac_sha256") == 0) return "hmac_sha256";
        if (strcmp(function, "verify_signature") == 0) return "verify_signature";
    } else if (strcmp(module, "atomic") == 0) {
        if (strcmp(function, "add") == 0) return "wyn_atomic_add";
        if (strcmp(function, "sub") == 0) return "wyn_atomic_sub";
        if (strcmp(function, "load") == 0) return "wyn_atomic_load";
        if (strcmp(function, "store") == 0) return "wyn_atomic_store";
        if (strcmp(function, "cas") == 0) return "wyn_atomic_cas";
    } else if (strcmp(module, "stringbuilder") == 0) {
        if (strcmp(function, "new") == 0) return "sb_new_wyn";
        if (strcmp(function, "append") == 0) return "sb_append_wyn";
        if (strcmp(function, "to_str") == 0) return "sb_to_str_wyn";
    }
    return NULL;
}

// Helper: Check if a call expression is calling a specific builtin
// Handles both direct calls (func()) and module calls (module.func())
static void parser_error(Parser* p, const char* fmt, ...) {
    p->had_error = true;
    fprintf(stderr, "\n");
    fprintf(stderr, "╭─ Parse Error in %s:%d:%d\n", p->filename, p->current.line, p->current.column);
    fprintf(stderr, "│\n");
    fprintf(stderr, "│  ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fprintf(stderr, "│\n");
    fprintf(stderr, "╰─ Syntax error\n");
    fprintf(stderr, "\n");
}

static Token parser_advance(Parser* p) {
    Token prev = p->current;
    p->current = lexer_next(p->lexer);
    // Track and skip newlines
    p->newline_before = false;
    while (p->current.kind == TOK_NEWLINE) {
        p->newline_before = true;
        p->current = lexer_next(p->lexer);
    }
    return prev;
}

static bool parser_check(Parser* p, TokenKind kind) {
    return p->current.kind == kind;
}

static bool parser_match(Parser* p, TokenKind kind) {
    if (parser_check(p, kind)) {
        parser_advance(p);
        return true;
    }
    return false;
}

static Token parser_expect(Parser* p, TokenKind kind, const char* msg) {
    if (p->current.kind == kind) {
        return parser_advance(p);
    }
    parser_error(p, "expected %s", msg);
    
    // Add helpful suggestions
    if (kind == TOK_LBRACE) {
        fprintf(stderr, "   💡 Missing opening brace '{' - check if/while/for/fn syntax\n");
    } else if (kind == TOK_RBRACE) {
        fprintf(stderr, "   💡 Missing closing brace '}' - check block structure\n");
    } else if (kind == TOK_LPAREN) {
        fprintf(stderr, "   💡 Missing opening parenthesis '(' - check function call\n");
    } else if (kind == TOK_RPAREN) {
        fprintf(stderr, "   💡 Missing closing parenthesis ')' - check expression\n");
    } else if (kind == TOK_COLON) {
        fprintf(stderr, "   💡 Missing colon ':' - check type annotation\n");
    }
    
    return p->current;
}

// Forward declarations
static Expr* parse_expr(Parser* p);
static Stmt* parse_stmt(Parser* p);
static Type* parse_type(Parser* p);

// Memory allocation helpers
static Expr* new_expr(ExprKind kind, int line, int col) {
    Expr* e = calloc(1, sizeof(Expr));
    e->kind = kind;
    e->line = line;
    e->column = col;
    return e;
}

static Stmt* new_stmt(StmtKind kind, int line, int col) {
    Stmt* s = calloc(1, sizeof(Stmt));
    s->kind = kind;
    s->line = line;
    s->column = col;
    return s;
}

static Type* new_type(TypeKind kind) {
    Type* t = calloc(1, sizeof(Type));
    t->kind = kind;
    return t;
}

// Helper: Get return type for builtin functions
static Type* get_builtin_return_type(const char* builtin_name) {
    // Array-returning functions
    if (strcmp(builtin_name, "str_split") == 0) {
        Type* arr = new_type(TYPE_ARRAY);
        arr->inner = new_type(TYPE_STR);
        return arr;
    }
    
    // String-returning functions
    if (strcmp(builtin_name, "read_file") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "os_exec_capture") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "path_join") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "path_dirname") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "path_basename") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "path_ext") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "format") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "str_format") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "format_timestamp") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "format_iso8601") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "base64_encode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "base64_decode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "hex_encode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "hex_decode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "url_encode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "url_decode") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "sha256_hash") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "md5_hash") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "sha1_hash") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "hashmap_new") == 0) return new_type(TYPE_ANY);
    if (strcmp(builtin_name, "hashmap_get") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "hashmap_keys") == 0) return new_type(TYPE_ANY);
    if (strcmp(builtin_name, "hashmap_values") == 0) return new_type(TYPE_ANY);
    if (strcmp(builtin_name, "set_new") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "encrypt_aes256") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "decrypt_aes256") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "generate_random_bytes") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "hmac_sha256") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "http_server_read_request") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "http_parse_method") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "http_parse_path") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "http_parse_body") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "exec_output_wyn") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "str_replace") == 0) return new_type(TYPE_STR);
    if (strcmp(builtin_name, "sb_to_str_wyn") == 0) return new_type(TYPE_STR);
    
    // Integer-returning functions
    if (strcmp(builtin_name, "now_unix") == 0) return new_type(TYPE_INT);
    if (strcmp(builtin_name, "now_millis") == 0) return new_type(TYPE_INT);
    if (strcmp(builtin_name, "now_micros") == 0) return new_type(TYPE_INT);
    if (strcmp(builtin_name, "parse_timestamp") == 0) return new_type(TYPE_INT);
    if (strcmp(builtin_name, "hashmap_contains") == 0) return new_type(TYPE_INT);
    if (strcmp(builtin_name, "hashmap_size") == 0) return new_type(TYPE_INT);
    
    // Void-returning functions
    if (strcmp(builtin_name, "log_info") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "log_warn") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "log_error") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "log_debug") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "log_with_level") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "sleep_seconds") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "sb_append_wyn") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "hashmap_put") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "hashmap_remove") == 0) return new_type(TYPE_VOID);
    if (strcmp(builtin_name, "hashmap_clear") == 0) return new_type(TYPE_VOID);
    
    // StringBuilder new returns opaque pointer (ANY type)
    if (strcmp(builtin_name, "sb_new_wyn") == 0) return new_type(TYPE_ANY);
    
    // Default to ANY for unknown functions
    return new_type(TYPE_ANY);
}

// Type parsing
static Type* parse_type(Parser* p) {
    Type* t = NULL;
    
    // Pointer type: *Type
    if (parser_match(p, TOK_STAR)) {
        t = new_type(TYPE_POINTER);
        t->inner = parse_type(p);
        return t;
    }
    
    // Optional type: ?Type
    if (parser_match(p, TOK_QUESTION)) {
        t = new_type(TYPE_OPTIONAL);
        t->inner = parse_type(p);
        return t;
    }
    
    // Array type: [Type]
    if (parser_match(p, TOK_LBRACKET)) {
        t = new_type(TYPE_ARRAY);
        t->inner = parse_type(p);
        parser_expect(p, TOK_RBRACKET, "]");
        return t;
    }
    
    // Map type: {KeyType: ValueType}
    if (parser_match(p, TOK_LBRACE)) {
        t = new_type(TYPE_MAP);
        t->key_type = parse_type(p);
        parser_expect(p, TOK_COLON, ":");
        t->value_type = parse_type(p);
        parser_expect(p, TOK_RBRACE, "}");
        return t;
    }
    
    // Tuple type: (Type, Type, ...)
    if (parser_match(p, TOK_LPAREN)) {
        t = new_type(TYPE_TUPLE);
        t->params = malloc(sizeof(Type*) * 16);
        t->param_count = 0;
        if (!parser_check(p, TOK_RPAREN)) {
            do {
                t->params[t->param_count++] = parse_type(p);
            } while (parser_match(p, TOK_COMMA));
        }
        parser_expect(p, TOK_RPAREN, ")");
        return t;
    }
    
    // Function type: fn(Type, Type) -> Type
    if (parser_match(p, TOK_FN)) {
        t = new_type(TYPE_FUNCTION);
        parser_expect(p, TOK_LPAREN, "(");
        t->params = malloc(sizeof(Type*) * MAX_PARAMS);
        t->param_count = 0;
        if (!parser_check(p, TOK_RPAREN)) {
            do {
                t->params[t->param_count++] = parse_type(p);
            } while (parser_match(p, TOK_COMMA));
        }
        parser_expect(p, TOK_RPAREN, ")");
        if (parser_match(p, TOK_ARROW)) {
            t->return_type = parse_type(p);
        }
        return t;
    }
    
    // Named type (including built-ins and generics)
    if (parser_check(p, TOK_IDENT)) {
        Token name = parser_advance(p);
        
        // Check built-in types
        if (strcmp(name.ident, "int") == 0) return new_type(TYPE_INT);
        if (strcmp(name.ident, "float") == 0) return new_type(TYPE_FLOAT);
        if (strcmp(name.ident, "str") == 0) return new_type(TYPE_STR);
        if (strcmp(name.ident, "bool") == 0) return new_type(TYPE_BOOL);
        if (strcmp(name.ident, "byte") == 0) return new_type(TYPE_BYTE);
        if (strcmp(name.ident, "array") == 0) return new_type(TYPE_ARRAY);
        if (strcmp(name.ident, "any") == 0) return new_type(TYPE_ANY);
        
        // Handle Result[T, E] specially
        if (strcmp(name.ident, "Result") == 0 && parser_match(p, TOK_LBRACKET)) {
            t = new_type(TYPE_RESULT);
            t->inner = parse_type(p);  // ok type
            parser_expect(p, TOK_COMMA, ",");
            t->value_type = parse_type(p);  // error type
            parser_expect(p, TOK_RBRACKET, "]");
            return t;
        }
        
        t = new_type(TYPE_NAMED);
        strcpy(t->name, name.ident);
        
        // Generic parameters: Type[T, U]
        if (parser_match(p, TOK_LBRACKET)) {
            t->kind = TYPE_GENERIC;
            t->params = malloc(sizeof(Type*) * MAX_GENERICS);
            t->param_count = 0;
            do {
                t->params[t->param_count++] = parse_type(p);
            } while (parser_match(p, TOK_COMMA));
            parser_expect(p, TOK_RBRACKET, "]");
        }
        return t;
    }
    
    parser_error(p, "expected type");
    return new_type(TYPE_ANY);
}

// Helper to check if string has interpolation ${expr} or {} placeholders
static bool has_interpolation(const char* s) {
    for (int i = 0; s[i]; i++) {
        if (s[i] == '$' && s[i+1] == '{') return true;
        if (s[i] == '{') {
            // Look for closing brace
            for (int j = i + 1; s[j]; j++) {
                if (s[j] == '}') return true;
            }
        }
    }
    return false;
}

// Parse interpolated string into concatenation expression
static Expr* parse_interpolated_string(Parser* p, const char* str, int line, int col) {
    Expr* result = NULL;
    char buf[MAX_STRING_LEN];
    int bi = 0;
    
    for (int i = 0; str[i]; i++) {
        if (str[i] == '$' && str[i+1] == '{') {
            // Flush literal part
            if (bi > 0) {
                buf[bi] = '\0';
                Expr* lit = new_expr(EXPR_STRING, line, col);
                strcpy(lit->str_val, buf);
                if (!result) result = lit;
                else {
                    Expr* cat = new_expr(EXPR_BINARY, line, col);
                    cat->binary.op = TOK_PLUS;
                    cat->binary.left = result;
                    cat->binary.right = lit;
                    result = cat;
                }
                bi = 0;
            }
            // Skip ${
            i += 2;
            // Extract variable name
            char var[256];
            int vi = 0;
            while (str[i] && str[i] != '}') {
                var[vi++] = str[i++];
            }
            var[vi] = '\0';
            
            // Create var.to_str() call for type conversion
            Expr* varexpr = new_expr(EXPR_IDENT, line, col);
            strcpy(varexpr->ident, var);
            
            Expr* field = new_expr(EXPR_FIELD, line, col);
            field->field.object = varexpr;
            strcpy(field->field.field, "to_str");
            
            Expr* call = new_expr(EXPR_CALL, line, col);
            call->call.func = field;
            call->call.args = NULL;
            call->call.arg_count = 0;
            
            if (!result) result = call;
            else {
                Expr* cat = new_expr(EXPR_BINARY, line, col);
                cat->binary.op = TOK_PLUS;
                cat->binary.left = result;
                cat->binary.right = call;
                result = cat;
            }
        } else {
            buf[bi++] = str[i];
        }
    }
    // Flush remaining literal
    if (bi > 0) {
        buf[bi] = '\0';
        Expr* lit = new_expr(EXPR_STRING, line, col);
        strcpy(lit->str_val, buf);
        if (!result) result = lit;
        else {
            Expr* cat = new_expr(EXPR_BINARY, line, col);
            cat->binary.op = TOK_PLUS;
            cat->binary.left = result;
            cat->binary.right = lit;
            result = cat;
        }
    }
    return result ? result : new_expr(EXPR_STRING, line, col);
}

// Parse template string with {var} placeholders into format call
static Expr* parse_template_string(Parser* p, const char* str, int line, int col) {
    // Check if this has named placeholders like {name}
    bool has_named_placeholders = false;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '{' && str[i+1] != '}') {
            // Look for closing brace
            for (int j = i + 1; str[j]; j++) {
                if (str[j] == '}') {
                    has_named_placeholders = true;
                    break;
                }
            }
        }
    }
    
    if (has_named_placeholders) {
        // Parse as interpolated string with {} instead of ${}
        Expr* result = NULL;
        char buf[MAX_STRING_LEN];
        int bi = 0;
        
        for (int i = 0; str[i]; i++) {
            if (str[i] == '{' && str[i+1] != '}') {
                // Flush literal part
                if (bi > 0) {
                    buf[bi] = '\0';
                    Expr* lit = new_expr(EXPR_STRING, line, col);
                    strcpy(lit->str_val, buf);
                    if (!result) result = lit;
                    else {
                        Expr* cat = new_expr(EXPR_BINARY, line, col);
                        cat->binary.op = TOK_PLUS;
                        cat->binary.left = result;
                        cat->binary.right = lit;
                        result = cat;
                    }
                    bi = 0;
                }
                // Skip {
                i++;
                // Extract variable name
                char var[256];
                int vi = 0;
                while (str[i] && str[i] != '}') {
                    var[vi++] = str[i++];
                }
                var[vi] = '\0';
                
                // Create variable reference directly (simplified approach)
                Expr* varexpr = new_expr(EXPR_IDENT, line, col);
                strcpy(varexpr->ident, var);
                
                if (!result) result = varexpr;
                else {
                    Expr* cat = new_expr(EXPR_BINARY, line, col);
                    cat->binary.op = TOK_PLUS;
                    cat->binary.left = result;
                    cat->binary.right = varexpr;
                    result = cat;
                }
            } else {
                buf[bi++] = str[i];
            }
        }
        // Flush remaining literal
        if (bi > 0) {
            buf[bi] = '\0';
            Expr* lit = new_expr(EXPR_STRING, line, col);
            strcpy(lit->str_val, buf);
            if (!result) result = lit;
            else {
                Expr* cat = new_expr(EXPR_BINARY, line, col);
                cat->binary.op = TOK_PLUS;
                cat->binary.left = result;
                cat->binary.right = lit;
                result = cat;
            }
        }
        return result ? result : new_expr(EXPR_STRING, line, col);
    } else {
        // Regular string with no placeholders
        Expr* e = new_expr(EXPR_STRING, line, col);
        strcpy(e->str_val, str);
        return e;
    }
}

// Expression parsing - precedence climbing
static Expr* parse_primary(Parser* p) {
    int line = p->current.line, col = p->current.column;
    
    // Literals
    if (parser_check(p, TOK_INT)) {
        Expr* e = new_expr(EXPR_INT, line, col);
        e->int_val = p->current.int_val;
        parser_advance(p);
        return e;
    }
    if (parser_check(p, TOK_FLOAT)) {
        Expr* e = new_expr(EXPR_FLOAT, line, col);
        e->float_val = p->current.float_val;
        parser_advance(p);
        return e;
    }
    if (parser_check(p, TOK_STRING) || parser_check(p, TOK_RAW_STRING) || parser_check(p, TOK_MULTILINE_STRING)) {
        char str[MAX_STRING_LEN];
        strcpy(str, p->current.str_val);
        TokenKind str_kind = p->current.kind;
        parser_advance(p);
        if (str_kind == TOK_STRING && has_interpolation(str)) {
            // Check if it's ${} interpolation or {} template
            bool has_dollar_interpolation = false;
            for (int i = 0; str[i]; i++) {
                if (str[i] == '$' && str[i+1] == '{') {
                    has_dollar_interpolation = true;
                    break;
                }
            }
            if (has_dollar_interpolation) {
                return parse_interpolated_string(p, str, line, col);
            } else {
                return parse_template_string(p, str, line, col);
            }
        }
        Expr* e = new_expr(EXPR_STRING, line, col);
        strcpy(e->str_val, str);
        return e;
    }
    if (parser_match(p, TOK_TRUE)) {
        Expr* e = new_expr(EXPR_BOOL, line, col);
        e->bool_val = true;
        return e;
    }
    if (parser_match(p, TOK_FALSE)) {
        Expr* e = new_expr(EXPR_BOOL, line, col);
        e->bool_val = false;
        return e;
    }
    if (parser_match(p, TOK_NONE)) {
        return new_expr(EXPR_NONE, line, col);
    }
    if (parser_match(p, TOK_SOME)) {
        Expr* e = new_expr(EXPR_SOME, line, col);
        parser_expect(p, TOK_LPAREN, "(");
        e->some.value = parse_expr(p);
        parser_expect(p, TOK_RPAREN, ")");
        return e;
    }
    if (parser_match(p, TOK_OK)) {
        Expr* e = new_expr(EXPR_OK, line, col);
        parser_expect(p, TOK_LPAREN, "(");
        e->some.value = parse_expr(p);
        parser_expect(p, TOK_RPAREN, ")");
        return e;
    }
    if (parser_match(p, TOK_ERR)) {
        Expr* e = new_expr(EXPR_ERR, line, col);
        parser_expect(p, TOK_LPAREN, "(");
        e->some.value = parse_expr(p);
        parser_expect(p, TOK_RPAREN, ")");
        return e;
    }
    
    // Identifier (including built-in functions like assert)
    if (parser_check(p, TOK_IDENT) || parser_check(p, TOK_SELF) || 
        parser_check(p, TOK_ASSERT)) {
        char name[MAX_IDENT_LEN];
        if (parser_check(p, TOK_ASSERT)) {
            strcpy(name, "assert");
        } else {
            strcpy(name, p->current.ident);
        }
        parser_advance(p);
        
        // Parse optional type arguments: Name[T, U]
        Type** type_args = NULL;
        int type_arg_count = 0;
        if (parser_check(p, TOK_LBRACKET) && name[0] >= 'A' && name[0] <= 'Z') {
            parser_advance(p);
            type_args = malloc(sizeof(Type*) * MAX_GENERICS);
            while (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_EOF)) {
                type_args[type_arg_count++] = parse_type(p);
                if (!parser_match(p, TOK_COMMA)) break;
            }
            parser_expect(p, TOK_RBRACKET, "]");
        }
        
        // Check for struct literal: Name { field: value, ... } or Name[T] { ... }
        // Only if name starts with uppercase (struct naming convention)
        // AND the next tokens look like a struct literal (ident: or })
        if (parser_check(p, TOK_LBRACE) && name[0] >= 'A' && name[0] <= 'Z') {
            // Save lexer state to peek ahead
            int saved_pos = p->lexer->pos;
            int saved_line = p->lexer->line;
            int saved_col = p->lexer->column;
            Token saved_current = p->current;
            
            parser_advance(p);  // consume {
            Token next = p->current;
            bool is_struct_lit = (next.kind == TOK_RBRACE) ||  // empty struct
                                 (next.kind == TOK_IDENT);
            if (is_struct_lit && next.kind == TOK_IDENT) {
                parser_advance(p);  // consume ident
                // Field shorthand: {field} or {field, ...} or {field: value}
                is_struct_lit = (p->current.kind == TOK_COLON || 
                                 p->current.kind == TOK_COMMA ||
                                 p->current.kind == TOK_RBRACE);
            }
            
            // Restore lexer state
            p->lexer->pos = saved_pos;
            p->lexer->line = saved_line;
            p->lexer->column = saved_col;
            p->current = saved_current;
            
            if (is_struct_lit) {
                parser_advance(p);  // consume { again
                Expr* e = new_expr(EXPR_STRUCT, line, col);
                strcpy(e->struct_lit.name, name);
                e->struct_lit.count = 0;
                e->struct_lit.type_args = type_args;
                e->struct_lit.type_arg_count = type_arg_count;
                if (!parser_check(p, TOK_RBRACE)) {
                    do {
                        Token fld = parser_expect(p, TOK_IDENT, "field name");
                        strcpy(e->struct_lit.fields[e->struct_lit.count], fld.ident);
                        if (parser_match(p, TOK_COLON)) {
                            e->struct_lit.values[e->struct_lit.count] = parse_expr(p);
                        } else {
                            // Field shorthand: {field} means {field: field}
                            Expr* ident = new_expr(EXPR_IDENT, fld.line, fld.column);
                            strcpy(ident->ident, fld.ident);
                            e->struct_lit.values[e->struct_lit.count] = ident;
                        }
                        e->struct_lit.count++;
                    } while (parser_match(p, TOK_COMMA) && !parser_check(p, TOK_RBRACE));
                }
                parser_expect(p, TOK_RBRACE, "}");
                return e;
            }
        }
        
        Expr* e = new_expr(EXPR_IDENT, line, col);
        strcpy(e->ident, name);
        return e;
    }
    
    // Parenthesized expr or tuple or unit type
    if (parser_match(p, TOK_LPAREN)) {
        // Check for unit type ()
        if (parser_check(p, TOK_RPAREN)) {
            parser_advance(p);
            // Return empty tuple as unit type
            Expr* e = new_expr(EXPR_TUPLE, line, col);
            e->array.elements = NULL;
            e->array.count = 0;
            return e;
        }
        
        Expr* first = parse_expr(p);
        if (parser_match(p, TOK_COMMA)) {
            // Tuple
            Expr* e = new_expr(EXPR_TUPLE, line, col);
            e->array.elements = malloc(sizeof(Expr*) * 16);
            e->array.elements[0] = first;
            e->array.count = 1;
            do {
                e->array.elements[e->array.count++] = parse_expr(p);
            } while (parser_match(p, TOK_COMMA));
            parser_expect(p, TOK_RPAREN, ")");
            return e;
        }
        parser_expect(p, TOK_RPAREN, ")");
        return first;
    }
    
    // Array literal or list comprehension
    if (parser_match(p, TOK_LBRACKET)) {
        // Check for empty array: []
        if (parser_check(p, TOK_RBRACKET)) {
            parser_advance(p);  // consume ]
            Expr* e = new_expr(EXPR_ARRAY, line, col);
            e->array.elements = malloc(sizeof(Expr*) * 256);
            e->array.count = 0;
            e->array.is_empty_literal = true;  // Mark as empty for type inference
            return e;
        }
        
        // Check if this is a list comprehension: [expr for var in iter]
        // We need to peek ahead to see if there's a 'for' keyword
        int saved_pos = p->lexer->pos;
        int saved_line = p->lexer->line;
        int saved_col = p->lexer->column;
        
        // Try to parse first element
        Expr* first_expr = parse_expr(p);
        
        // Check if 'for' follows
        if (parser_check(p, TOK_FOR)) {
            // This is a list comprehension
            parser_advance(p);  // consume 'for'
            
            Expr* comp = new_expr(EXPR_COMPREHENSION, line, col);
            comp->comprehension.expr = first_expr;
            
            Token var = parser_expect(p, TOK_IDENT, "variable name");
            strcpy(comp->comprehension.var, var.ident);
            
            parser_expect(p, TOK_IN, "in");
            comp->comprehension.iter = parse_expr(p);
            
            // Optional 'if' condition
            if (parser_match(p, TOK_IF)) {
                comp->comprehension.condition = parse_expr(p);
            } else {
                comp->comprehension.condition = NULL;
            }
            
            parser_expect(p, TOK_RBRACKET, "]");
            return comp;
        } else {
            // Regular array literal
            Expr* e = new_expr(EXPR_ARRAY, line, col);
            e->array.elements = malloc(sizeof(Expr*) * 256);
            e->array.count = 0;
            e->array.is_empty_literal = false;
            e->array.elements[e->array.count++] = first_expr;
            
            while (parser_match(p, TOK_COMMA)) {
                if (parser_check(p, TOK_RBRACKET)) break;
                e->array.elements[e->array.count++] = parse_expr(p);
            }
            parser_expect(p, TOK_RBRACKET, "]");
            return e;
        }
    }
    
    // Map literal
    if (parser_match(p, TOK_LBRACE)) {
        Expr* e = new_expr(EXPR_MAP, line, col);
        e->map.keys = malloc(sizeof(char*) * 64);
        e->map.values = malloc(sizeof(Expr*) * 64);
        e->map.count = 0;
        if (!parser_check(p, TOK_RBRACE)) {
            do {
                if (parser_check(p, TOK_STRING)) {
                    e->map.keys[e->map.count] = strdup(p->current.str_val);
                    parser_advance(p);
                } else if (parser_check(p, TOK_IDENT)) {
                    e->map.keys[e->map.count] = strdup(p->current.ident);
                    parser_advance(p);
                }
                parser_expect(p, TOK_COLON, ":");
                e->map.values[e->map.count] = parse_expr(p);
                e->map.count++;
            } while (parser_match(p, TOK_COMMA));
        }
        parser_expect(p, TOK_RBRACE, "}");
        return e;
    }
    
    // Match expression
    if (parser_match(p, TOK_MATCH)) {
        Expr* e = new_expr(EXPR_MATCH, line, col);
        e->match_expr.value = parse_expr(p);
        e->match_expr.patterns = malloc(sizeof(Expr*) * 64);
        e->match_expr.arms = malloc(sizeof(Expr*) * 64);
        e->match_expr.binding_types = malloc(sizeof(Type*) * 64);
        e->match_expr.arm_count = 0;
        parser_expect(p, TOK_LBRACE, "{");
        while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
            Expr* pattern = parse_expr(p);
            e->match_expr.patterns[e->match_expr.arm_count] = pattern;
            e->match_expr.bindings[e->match_expr.arm_count][0] = '\0';
            e->match_expr.binding_types[e->match_expr.arm_count] = NULL;
            if ((pattern->kind == EXPR_SOME || pattern->kind == EXPR_OK || pattern->kind == EXPR_ERR) &&
                pattern->some.value && pattern->some.value->kind == EXPR_IDENT) {
                strcpy(e->match_expr.bindings[e->match_expr.arm_count], pattern->some.value->ident);
            }
            parser_expect(p, TOK_FATARROW, "=>");
            e->match_expr.arms[e->match_expr.arm_count] = parse_expr(p);
            e->match_expr.arm_count++;
        }
        parser_expect(p, TOK_RBRACE, "}");
        return e;
    }
    
    // Lambda: fn(params) { body } or fn(params) -> Type { body }
    if (parser_match(p, TOK_FN)) {
        Expr* e = new_expr(EXPR_LAMBDA, line, col);
        e->lambda.param_count = 0;
        e->lambda.return_type = NULL;
        e->lambda.id = p->lambda_count++;
        
        parser_expect(p, TOK_LPAREN, "(");
        while (!parser_check(p, TOK_RPAREN) && !parser_check(p, TOK_EOF)) {
            if (e->lambda.param_count > 0) parser_expect(p, TOK_COMMA, ",");
            Token name = parser_expect(p, TOK_IDENT, "parameter name");
            strcpy(e->lambda.params[e->lambda.param_count], name.ident);
            parser_expect(p, TOK_COLON, ":");
            e->lambda.param_types[e->lambda.param_count] = parse_type(p);
            e->lambda.param_count++;
        }
        parser_expect(p, TOK_RPAREN, ")");
        
        if (parser_match(p, TOK_ARROW)) {
            e->lambda.return_type = parse_type(p);
        }
        
        parser_expect(p, TOK_LBRACE, "{");
        e->lambda.body = malloc(sizeof(Stmt*) * 256);
        e->lambda.body_count = 0;
        while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
            e->lambda.body[e->lambda.body_count++] = parse_stmt(p);
        }
        parser_expect(p, TOK_RBRACE, "}");
        return e;
    }
    
    parser_error(p, "expected expression, got token %d", p->current.kind);
    parser_advance(p);  // Skip the bad token to avoid infinite loop
    return new_expr(EXPR_INT, line, col);
}

static Expr* parse_postfix(Parser* p) {
    Expr* e = parse_primary(p);
    
    while (true) {
        int line = p->current.line, col = p->current.column;
        
        // Generic function call: func[T](args) - detect by checking if next is type-like
        if (parser_check(p, TOK_LBRACKET) && e->kind == EXPR_IDENT) {
            // Peek ahead: if next token after [ is a type keyword or uppercase ident, it's generic
            parser_advance(p);  // consume [
            
            bool is_type = parser_check(p, TOK_IDENT) && 
                ((p->current.ident[0] >= 'A' && p->current.ident[0] <= 'Z') ||
                 strcmp(p->current.ident, "int") == 0 ||
                 strcmp(p->current.ident, "str") == 0 ||
                 strcmp(p->current.ident, "bool") == 0 ||
                 strcmp(p->current.ident, "float") == 0 ||
                 strcmp(p->current.ident, "byte") == 0);
            
            if (is_type) {
                Type** type_args = malloc(sizeof(Type*) * MAX_GENERICS);
                int type_arg_count = 0;
                
                while (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_EOF)) {
                    type_args[type_arg_count++] = parse_type(p);
                    if (!parser_match(p, TOK_COMMA)) break;
                }
                parser_expect(p, TOK_RBRACKET, "]");
                
                if (parser_match(p, TOK_LPAREN)) {
                    // Generic function call
                    Expr* call = new_expr(EXPR_CALL, line, col);
                    call->call.func = e;
                    call->call.args = malloc(sizeof(Expr*) * MAX_PARAMS);
                    call->call.arg_count = 0;
                    call->call.type_args = type_args;
                    call->call.type_arg_count = type_arg_count;
                    if (!parser_check(p, TOK_RPAREN)) {
                        do {
                            call->call.args[call->call.arg_count++] = parse_expr(p);
                        } while (parser_match(p, TOK_COMMA));
                    }
                    parser_expect(p, TOK_RPAREN, ")");
                    e = call;
                    continue;
                }
                // Not followed by (, treat as error or continue
                free(type_args);
            } else {
                // Not a type, parse as index/slice - put back the [
                // We already consumed [, so continue with index parsing
                Expr* start = NULL;
                Expr* end = NULL;
                Expr* step = NULL;
                bool is_slice = false;
                
                if (parser_check(p, TOK_COLON)) {
                    is_slice = true;
                } else if (!parser_check(p, TOK_RBRACKET)) {
                    start = parse_expr(p);
                }
                
                if (parser_match(p, TOK_COLON)) {
                    is_slice = true;
                    if (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_COLON)) {
                        end = parse_expr(p);
                    }
                    if (parser_match(p, TOK_COLON)) {
                        if (!parser_check(p, TOK_RBRACKET)) {
                            step = parse_expr(p);
                        }
                    }
                }
                parser_expect(p, TOK_RBRACKET, "]");
                
                if (is_slice) {
                    Expr* slice = new_expr(EXPR_SLICE, line, col);
                    slice->slice.object = e;
                    slice->slice.start = start;
                    slice->slice.end = end;
                    slice->slice.step = step;
                    e = slice;
                } else {
                    Expr* idx = new_expr(EXPR_INDEX, line, col);
                    idx->index.object = e;
                    idx->index.index = start;
                    idx->index.safe = false;
                    e = idx;
                }
                continue;
            }
        }
        
        // Function call: expr(args)
        if (parser_match(p, TOK_LPAREN)) {
            Expr* call = new_expr(EXPR_CALL, line, col);
            call->call.func = e;
            call->call.args = malloc(sizeof(Expr*) * MAX_PARAMS);
            call->call.arg_count = 0;
            call->call.type_args = NULL;
            call->call.type_arg_count = 0;
            if (!parser_check(p, TOK_RPAREN)) {
                do {
                    call->call.args[call->call.arg_count++] = parse_expr(p);
                } while (parser_match(p, TOK_COMMA));
            }
            parser_expect(p, TOK_RPAREN, ")");
            e = call;
            continue;
        }
        
        // Index or Slice: expr[index] or expr[start:end] or expr[start:end:step]
        if (parser_match(p, TOK_LBRACKET)) {
            Expr* start = NULL;
            Expr* end = NULL;
            Expr* step = NULL;
            bool is_slice = false;
            
            // Check for [:...] (empty start)
            if (parser_check(p, TOK_COLON)) {
                is_slice = true;
            } else if (!parser_check(p, TOK_RBRACKET)) {
                start = parse_expr(p);
            }
            
            // Check for slice syntax
            if (parser_match(p, TOK_COLON)) {
                is_slice = true;
                // Parse end if present
                if (!parser_check(p, TOK_COLON) && !parser_check(p, TOK_RBRACKET)) {
                    end = parse_expr(p);
                }
                // Check for step
                if (parser_match(p, TOK_COLON)) {
                    if (!parser_check(p, TOK_RBRACKET)) {
                        step = parse_expr(p);
                    }
                }
            }
            
            parser_expect(p, TOK_RBRACKET, "]");
            
            if (is_slice) {
                Expr* sl = new_expr(EXPR_SLICE, line, col);
                sl->slice.object = e;
                sl->slice.start = start;
                sl->slice.end = end;
                sl->slice.step = step;
                e = sl;
            } else {
                Expr* idx = new_expr(EXPR_INDEX, line, col);
                idx->index.object = e;
                idx->index.index = start;
                idx->index.safe = false;
                e = idx;
            }
            continue;
        }
        
        // Field access: expr.field
        if (parser_match(p, TOK_DOT)) {
            Expr* fld = new_expr(EXPR_FIELD, line, col);
            fld->field.object = e;
            Token name = parser_expect(p, TOK_IDENT, "field name");
            strcpy(fld->field.field, name.ident);
            e = fld;
            continue;
        }
        
        // Optional chaining: expr?.field
        if (parser_match(p, TOK_QUESTIONDOT)) {
            Expr* fld = new_expr(EXPR_FIELD, line, col);
            fld->field.object = e;
            Token name = parser_expect(p, TOK_IDENT, "field name");
            strcpy(fld->field.field, name.ident);
            e = fld;
            continue;
        }
        
        // Unwrap: expr!
        if (parser_match(p, TOK_BANG)) {
            Expr* unwrap = new_expr(EXPR_UNWRAP, line, col);
            unwrap->unary.operand = e;
            unwrap->unary.op = TOK_BANG;
            e = unwrap;
            continue;
        }
        
        // Try: expr? (only if not followed by expression - that would be ternary)
        if (parser_check(p, TOK_QUESTION)) {
            // Peek ahead - if next token is an expression start, this is ternary, not try
            Token next = lexer_peek(p->lexer);
            bool is_expr_start = (next.kind == TOK_INT || next.kind == TOK_STRING || 
                                 next.kind == TOK_IDENT || next.kind == TOK_LPAREN ||
                                 next.kind == TOK_LBRACKET || next.kind == TOK_TRUE ||
                                 next.kind == TOK_FALSE || next.kind == TOK_MINUS ||
                                 next.kind == TOK_NOT);
            
            if (!is_expr_start) {
                // This is try operator
                parser_advance(p);
                Expr* try_e = new_expr(EXPR_TRY, line, col);
                try_e->unary.operand = e;
                try_e->unary.op = TOK_QUESTION;
                e = try_e;
                continue;
            }
            // Otherwise, leave it for ternary parsing in parse_expr
        }
        
        break;
    }
    return e;
}

static Expr* parse_unary(Parser* p) {
    int line = p->current.line, col = p->current.column;
    
    // Address-of: &expr
    if (parser_match(p, TOK_AMPERSAND)) {
        Expr* e = new_expr(EXPR_ADDR, line, col);
        e->unary.operand = parse_unary(p);
        return e;
    }
    
    // Dereference: *expr
    if (parser_match(p, TOK_STAR)) {
        Expr* e = new_expr(EXPR_DEREF, line, col);
        e->unary.operand = parse_unary(p);
        return e;
    }
    
    if (parser_check(p, TOK_MINUS) || parser_check(p, TOK_NOT) || 
        parser_check(p, TOK_BANG) || parser_check(p, TOK_TILDE)) {
        TokenKind op = p->current.kind;
        parser_advance(p);
        Expr* e = new_expr(EXPR_UNARY, line, col);
        e->unary.op = op;
        e->unary.operand = parse_unary(p);
        return e;
    }
    return parse_postfix(p);
}

static int get_precedence(TokenKind op) {
    switch (op) {
        case TOK_PIPEGT: return 0;      // pipe operator (lowest precedence)
        case TOK_OR: return 1;
        case TOK_AND: return 2;
        case TOK_PIPE: return 3;        // bitwise or
        case TOK_CARET: return 4;       // bitwise xor
        case TOK_AMPERSAND: return 5;   // bitwise and
        case TOK_EQEQ: case TOK_NOTEQ: case TOK_IN: return 6;  // in is comparison-level
        case TOK_LT: case TOK_GT: case TOK_LTEQ: case TOK_GTEQ: return 7;
        case TOK_LTLT: case TOK_GTGT: return 8;  // shifts
        case TOK_DOTDOT: case TOK_DOTDOTEQ: return 9;  // range operators
        case TOK_PLUS: case TOK_MINUS: return 10;
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return 11;
        case TOK_STARSTAR: return 12;
        case TOK_QUESTIONQUESTION: return 0;
        default: return -1;
    }
}

static Expr* parse_binary(Parser* p, int min_prec) {
    Expr* left = parse_unary(p);
    
    while (true) {
        // Stop at newline for operators that could start a new statement
        // This prevents *p on a new line from being parsed as multiplication
        if (p->newline_before && (p->current.kind == TOK_STAR || 
                                   p->current.kind == TOK_MINUS ||
                                   p->current.kind == TOK_AMPERSAND)) {
            break;
        }
        
        int prec = get_precedence(p->current.kind);
        if (prec < min_prec) break;
        
        int line = p->current.line, col = p->current.column;
        TokenKind op = p->current.kind;
        parser_advance(p);
        
        Expr* right = parse_binary(p, prec + 1);
        
        if (op == TOK_PIPEGT) {
            // Transform x |> f into f(x)
            Expr* call = new_expr(EXPR_CALL, line, col);
            call->call.func = right;
            call->call.args = malloc(sizeof(Expr*));
            call->call.args[0] = left;
            call->call.arg_count = 1;
            left = call;
        } else if (op == TOK_QUESTIONQUESTION) {
            Expr* e = new_expr(EXPR_DEFAULT, line, col);
            e->default_expr.value = left;
            e->default_expr.default_val = right;
            left = e;
        } else {
            Expr* e = new_expr(EXPR_BINARY, line, col);
            e->binary.left = left;
            e->binary.right = right;
            e->binary.op = op;
            left = e;
        }
    }
    return left;
}

static Expr* parse_expr(Parser* p) {
    Expr* expr = parse_binary(p, 0);
    
    // Check for ternary operator: condition ? true_val : false_val
    if (parser_check(p, TOK_QUESTION)) {
        parser_advance(p);  // consume ?
        int line = p->current.line, col = p->current.column;
        
        // Parse true value
        Expr* true_val = parse_unary(p);
        
        if (!parser_check(p, TOK_COLON)) {
            parser_error(p, "expected ':' in ternary expression");
        }
        parser_advance(p);  // consume :
        
        // Parse false value (can be another ternary)
        Expr* false_val = parse_expr(p);
        
        Expr* ternary = new_expr(EXPR_TERNARY, line, col);
        ternary->ternary.cond = expr;
        ternary->ternary.true_val = true_val;
        ternary->ternary.false_val = false_val;
        return ternary;
    }
    
    return expr;
}

// Statement parsing
static Stmt** parse_block(Parser* p, int* count) {
    Stmt** stmts = malloc(sizeof(Stmt*) * 256);
    *count = 0;
    parser_expect(p, TOK_LBRACE, "{");
    while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
        stmts[(*count)++] = parse_stmt(p);
    }
    parser_expect(p, TOK_RBRACE, "}");
    return stmts;
}

static Stmt* parse_stmt(Parser* p) {
    int line = p->current.line, col = p->current.column;
    
    // Variable declaration: var (mutable) or const (immutable)
    if (parser_check(p, TOK_VAR) || parser_check(p, TOK_CONST) ||
        (parser_check(p, TOK_PUB) && (lexer_peek(p->lexer).kind == TOK_VAR || lexer_peek(p->lexer).kind == TOK_CONST))) {
        Stmt* s = new_stmt(STMT_LET, line, col);
        s->let.is_pub = parser_match(p, TOK_PUB);
        
        // var = mutable, const = immutable
        if (parser_match(p, TOK_VAR)) {
            s->let.is_mut = true;
        } else if (parser_match(p, TOK_CONST)) {
            s->let.is_mut = false;
        }
        
        Token name = parser_expect(p, TOK_IDENT, "variable name");
        strcpy(s->let.name, name.ident);
        
        if (parser_match(p, TOK_COLON)) {
            s->let.type = parse_type(p);
        }
        if (parser_match(p, TOK_EQ)) {
            s->let.value = parse_expr(p);
        }
        return s;
    }
    
    // Return statement
    if (parser_match(p, TOK_RETURN)) {
        Stmt* s = new_stmt(STMT_RETURN, line, col);
        if (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_NEWLINE) && !parser_check(p, TOK_EOF)) {
            s->ret.value = parse_expr(p);
        }
        return s;
    }
    
    // Break/Continue
    if (parser_match(p, TOK_BREAK)) {
        return new_stmt(STMT_BREAK, line, col);
    }
    if (parser_match(p, TOK_CONTINUE)) {
        return new_stmt(STMT_CONTINUE, line, col);
    }
    
    // Spawn
    if (parser_match(p, TOK_SPAWN)) {
        Stmt* s = new_stmt(STMT_SPAWN, line, col);
        s->spawn.body = parse_block(p, &s->spawn.body_count);
        s->spawn.captured_vars = NULL;
        s->spawn.captured_count = 0;
        return s;
    }
    
    // If statement
    if (parser_match(p, TOK_IF)) {
        Stmt* s = new_stmt(STMT_IF, line, col);
        s->if_stmt.cond = parse_expr(p);
        s->if_stmt.then_block = parse_block(p, &s->if_stmt.then_count);
        if (parser_match(p, TOK_ELSE)) {
            if (parser_check(p, TOK_IF)) {
                // else if - wrap in block
                s->if_stmt.else_block = malloc(sizeof(Stmt*));
                s->if_stmt.else_block[0] = parse_stmt(p);
                s->if_stmt.else_count = 1;
            } else {
                s->if_stmt.else_block = parse_block(p, &s->if_stmt.else_count);
            }
        }
        return s;
    }
    
    // While statement
    if (parser_match(p, TOK_WHILE)) {
        Stmt* s = new_stmt(STMT_WHILE, line, col);
        s->while_stmt.cond = parse_expr(p);
        s->while_stmt.body = parse_block(p, &s->while_stmt.body_count);
        return s;
    }
    
    // For statement
    if (parser_check(p, TOK_FOR)) {
        Stmt* s = new_stmt(STMT_FOR, line, col);
        s->for_stmt.is_parallel = false;
        parser_expect(p, TOK_FOR, "for");
        Token var = parser_expect(p, TOK_IDENT, "variable name");
        strcpy(s->for_stmt.var, var.ident);
        parser_expect(p, TOK_IN, "in");
        s->for_stmt.iter = parse_expr(p);
        s->for_stmt.body = parse_block(p, &s->for_stmt.body_count);
        return s;
    }
    
    // Match statement
    if (parser_match(p, TOK_MATCH)) {
        Stmt* s = new_stmt(STMT_MATCH, line, col);
        s->match_stmt.value = parse_expr(p);
        s->match_stmt.patterns = malloc(sizeof(Expr*) * 64);
        s->match_stmt.arms = malloc(sizeof(Stmt**) * 64);
        s->match_stmt.arm_counts = malloc(sizeof(int) * 64);
        s->match_stmt.binding_types = malloc(sizeof(Type*) * 64);
        s->match_stmt.arm_count = 0;
        parser_expect(p, TOK_LBRACE, "{");
        while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
            // Parse pattern (expression or _ for wildcard)
            Expr* pattern = parse_expr(p);
            s->match_stmt.patterns[s->match_stmt.arm_count] = pattern;
            // Extract binding variable from patterns like some(x), ok(v), err(e)
            s->match_stmt.bindings[s->match_stmt.arm_count][0] = '\0';
            s->match_stmt.binding_types[s->match_stmt.arm_count] = NULL;
            if ((pattern->kind == EXPR_SOME || pattern->kind == EXPR_OK || pattern->kind == EXPR_ERR) &&
                pattern->some.value && pattern->some.value->kind == EXPR_IDENT) {
                strcpy(s->match_stmt.bindings[s->match_stmt.arm_count], pattern->some.value->ident);
            }
            parser_expect(p, TOK_FATARROW, "=>");
            // Parse arm body - either block or single statement
            if (parser_check(p, TOK_LBRACE)) {
                s->match_stmt.arms[s->match_stmt.arm_count] = parse_block(p, &s->match_stmt.arm_counts[s->match_stmt.arm_count]);
            } else {
                s->match_stmt.arms[s->match_stmt.arm_count] = malloc(sizeof(Stmt*));
                s->match_stmt.arms[s->match_stmt.arm_count][0] = parse_stmt(p);
                s->match_stmt.arm_counts[s->match_stmt.arm_count] = 1;
            }
            s->match_stmt.arm_count++;
        }
        parser_expect(p, TOK_RBRACE, "}");
        return s;
    }
    
    // Block
    if (parser_check(p, TOK_LBRACE)) {
        Stmt* s = new_stmt(STMT_BLOCK, line, col);
        s->block.stmts = parse_block(p, &s->block.count);
        return s;
    }
    
    // Special handling for pointer dereference assignment: *ident = value
    if (parser_check(p, TOK_STAR)) {
        parser_advance(p);  // consume *
        Expr* target = new_expr(EXPR_DEREF, line, col);
        // Parse just the identifier or parenthesized expression
        if (parser_check(p, TOK_IDENT)) {
            Expr* ident = new_expr(EXPR_IDENT, p->current.line, p->current.column);
            strcpy(ident->ident, p->current.ident);
            parser_advance(p);
            target->unary.operand = ident;
        } else if (parser_check(p, TOK_LPAREN)) {
            parser_advance(p);
            target->unary.operand = parse_expr(p);
            parser_expect(p, TOK_RPAREN, ")");
        } else {
            target->unary.operand = parse_primary(p);
        }
        
        if (parser_check(p, TOK_EQ)) {
            Stmt* s = new_stmt(STMT_ASSIGN, line, col);
            s->assign.target = target;
            s->assign.op = TOK_EQ;
            parser_advance(p);  // consume =
            s->assign.value = parse_expr(p);
            return s;
        }
        // Not an assignment, wrap in expression statement
        Stmt* s = new_stmt(STMT_EXPR, line, col);
        s->expr.expr = target;
        return s;
    }
    
    // Expression statement (or assignment)
    Expr* expr = parse_expr(p);
    
    // Check for assignment
    if (parser_check(p, TOK_EQ) || parser_check(p, TOK_PLUSEQ) || parser_check(p, TOK_MINUSEQ) ||
        parser_check(p, TOK_STAREQ) || parser_check(p, TOK_SLASHEQ) || parser_check(p, TOK_PERCENTEQ)) {
        Stmt* s = new_stmt(STMT_ASSIGN, line, col);
        s->assign.target = expr;
        s->assign.op = p->current.kind;
        parser_advance(p);
        s->assign.value = parse_expr(p);
        return s;
    }
    
    Stmt* s = new_stmt(STMT_EXPR, line, col);
    s->expr.expr = expr;
    return s;
}

// Top-level parsing
static FnDef* parse_function(Parser* p, bool is_pub, bool is_async) {
    FnDef* fn = calloc(1, sizeof(FnDef));
    fn->is_pub = is_pub;
    fn->is_async = is_async;
    
    parser_expect(p, TOK_FN, "fn");
    Token name = parser_expect(p, TOK_IDENT, "function name");
    
    // Check for extension method: fn TypeName.method_name
    if (parser_match(p, TOK_DOT)) {
        // This is an extension method
        char type_name[MAX_IDENT_LEN];
        strcpy(type_name, name.ident);
        
        Token method_name = parser_expect(p, TOK_IDENT, "method name");
        // Store as "TypeName__method_name" internally
        snprintf(fn->name, MAX_IDENT_LEN, "%s__%s", type_name, method_name.ident);
        fn->is_extension = true;
        strcpy(fn->extension_type, type_name);
        strcpy(fn->extension_method, method_name.ident);
    } else {
        strcpy(fn->name, name.ident);
    }
    
    // Generic parameters
    if (parser_match(p, TOK_LBRACKET)) {
        while (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_EOF)) {
            Token g = parser_expect(p, TOK_IDENT, "generic parameter");
            strcpy(fn->generics[fn->generic_count++], g.ident);
            if (!parser_match(p, TOK_COMMA)) break;
        }
        parser_expect(p, TOK_RBRACKET, "]");
    }
    
    // Parameters
    parser_expect(p, TOK_LPAREN, "(");
    if (!parser_check(p, TOK_RPAREN)) {
        do {
            // All parameters are mutable by default
            // Handle 'self' as a special parameter
            if (parser_check(p, TOK_SELF)) {
                parser_advance(p);
                strcpy(fn->params[fn->param_count].name, "self");
            } else {
                Token pname = parser_expect(p, TOK_IDENT, "parameter name");
                strcpy(fn->params[fn->param_count].name, pname.ident);
            }
            if (parser_match(p, TOK_COLON)) {
                // Check for variadic: ...Type
                if (parser_match(p, TOK_DOTDOTDOT)) {
                    fn->params[fn->param_count].is_variadic = true;
                    Type* inner = parse_type(p);
                    Type* arr = new_type(TYPE_ARRAY);
                    arr->inner = inner;
                    fn->params[fn->param_count].type = arr;
                } else {
                    fn->params[fn->param_count].type = parse_type(p);
                }
            }
            if (parser_match(p, TOK_EQ)) {
                fn->params[fn->param_count].default_val = parse_expr(p);
            }
            fn->param_count++;
        } while (parser_match(p, TOK_COMMA));
    }
    parser_expect(p, TOK_RPAREN, ")");
    
    // Return type
    if (parser_match(p, TOK_ARROW)) {
        fn->return_type = parse_type(p);
    }
    
    // Body
    fn->body = parse_block(p, &fn->body_count);
    return fn;
}

static StructDef* parse_struct(Parser* p, bool is_pub) {
    StructDef* s = calloc(1, sizeof(StructDef));
    s->is_pub = is_pub;
    
    parser_expect(p, TOK_STRUCT, "struct");
    Token name = parser_expect(p, TOK_IDENT, "struct name");
    strcpy(s->name, name.ident);
    
    // Generic parameters
    if (parser_match(p, TOK_LBRACKET)) {
        while (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_EOF)) {
            Token g = parser_expect(p, TOK_IDENT, "generic parameter");
            strcpy(s->generics[s->generic_count++], g.ident);
            if (!parser_match(p, TOK_COMMA)) break;
        }
        parser_expect(p, TOK_RBRACKET, "]");
    }
    
    parser_expect(p, TOK_LBRACE, "{");
    s->methods = malloc(sizeof(FnDef) * 64);
    
    while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
        while (parser_match(p, TOK_NEWLINE));
        if (parser_check(p, TOK_RBRACE)) break;
        
        bool field_pub = parser_match(p, TOK_PUB);
        
        if (parser_check(p, TOK_FN)) {
            FnDef* method = parse_function(p, field_pub, false);
            s->methods[s->method_count++] = *method;
            free(method);
        } else if (parser_check(p, TOK_IDENT)) {
            Token fname = parser_advance(p);
            strcpy(s->fields[s->field_count].name, fname.ident);
            s->fields[s->field_count].is_pub = field_pub;
            if (parser_match(p, TOK_COLON)) {
                s->fields[s->field_count].type = parse_type(p);
            }
            s->field_count++;
            parser_match(p, TOK_COMMA);
            while (parser_match(p, TOK_NEWLINE));
        }
    }
    parser_expect(p, TOK_RBRACE, "}");
    return s;
}

static EnumDef* parse_enum(Parser* p, bool is_pub) {
    EnumDef* e = calloc(1, sizeof(EnumDef));
    e->is_pub = is_pub;
    
    parser_expect(p, TOK_ENUM, "enum");
    Token name = parser_expect(p, TOK_IDENT, "enum name");
    strcpy(e->name, name.ident);
    
    // Generic parameters
    if (parser_match(p, TOK_LBRACKET)) {
        while (!parser_check(p, TOK_RBRACKET) && !parser_check(p, TOK_EOF)) {
            Token g = parser_expect(p, TOK_IDENT, "generic parameter");
            strcpy(e->generics[e->generic_count++], g.ident);
            if (!parser_match(p, TOK_COMMA)) break;
        }
        parser_expect(p, TOK_RBRACKET, "]");
    }
    
    parser_expect(p, TOK_LBRACE, "{");
    while (!parser_check(p, TOK_RBRACE) && !parser_check(p, TOK_EOF)) {
        // Skip newlines
        while (parser_match(p, TOK_NEWLINE));
        if (parser_check(p, TOK_RBRACE)) break;
        
        Token vname = parser_expect(p, TOK_IDENT, "variant name");
        strcpy(e->variants[e->variant_count].name, vname.ident);
        
        // Associated types
        if (parser_match(p, TOK_LPAREN)) {
            e->variants[e->variant_count].types = malloc(sizeof(Type*) * 8);
            do {
                e->variants[e->variant_count].types[e->variants[e->variant_count].type_count++] = parse_type(p);
            } while (parser_match(p, TOK_COMMA));
            parser_expect(p, TOK_RPAREN, ")");
        }
        
        // Explicit value
        if (parser_match(p, TOK_EQ)) {
            e->variants[e->variant_count].has_value = true;
            e->variants[e->variant_count].value = p->current.int_val;
            parser_advance(p);
        }
        e->variant_count++;
        
        // Skip comma and newlines
        parser_match(p, TOK_COMMA);
        while (parser_match(p, TOK_NEWLINE));
    }
    parser_expect(p, TOK_RBRACE, "}");
    return e;
}

// Parse method signature (for interfaces - no body)
static FnDef* parse_method_sig(Parser* p) {
    FnDef* fn = calloc(1, sizeof(FnDef));
    
    parser_expect(p, TOK_FN, "fn");
    Token name = parser_expect(p, TOK_IDENT, "method name");
    strcpy(fn->name, name.ident);
    
    parser_expect(p, TOK_LPAREN, "(");
    if (!parser_check(p, TOK_RPAREN)) {
        do {
            // All parameters are mutable by default
            if (parser_check(p, TOK_SELF)) {
                parser_advance(p);
                strcpy(fn->params[fn->param_count].name, "self");
            } else {
                Token pname = parser_expect(p, TOK_IDENT, "parameter name");
                strcpy(fn->params[fn->param_count].name, pname.ident);
            }
            if (parser_match(p, TOK_COLON)) {
                fn->params[fn->param_count].type = parse_type(p);
            }
            fn->param_count++;
        } while (parser_match(p, TOK_COMMA));
    }
    parser_expect(p, TOK_RPAREN, ")");
    
    if (parser_match(p, TOK_ARROW)) {
        fn->return_type = parse_type(p);
    }
    
    return fn;
}

static TestDef* parse_test(Parser* p) {
    TestDef* t = calloc(1, sizeof(TestDef));
    parser_expect(p, TOK_TEST, "test");
    Token name = parser_expect(p, TOK_STRING, "test name");
    strcpy(t->name, name.str_val);
    t->body = parse_block(p, &t->body_count);
    return t;
}

static Import* parse_import(Parser* p) {
    Import* imp = calloc(1, sizeof(Import));
    parser_expect(p, TOK_IMPORT, "import");
    
    // Selective: import { a, b } from path
    if (parser_match(p, TOK_LBRACE)) {
        imp->imports = malloc(sizeof(char*) * 32);
        do {
            Token name = parser_expect(p, TOK_IDENT, "import name");
            imp->imports[imp->import_count++] = strdup(name.ident);
        } while (parser_match(p, TOK_COMMA));
        parser_expect(p, TOK_RBRACE, "}");
        parser_expect(p, TOK_FROM, "from");
    }
    
    // Path (identifier or ./path or url)
    if (parser_check(p, TOK_IDENT)) {
        strcpy(imp->path, p->current.ident);
        parser_advance(p);
        // Handle std/module, module/submodule, or module.submodule syntax
        while (parser_check(p, TOK_SLASH) || parser_check(p, TOK_DOT)) {
            if (parser_check(p, TOK_SLASH)) {
                strcat(imp->path, "/");
                parser_advance(p);
            } else if (parser_check(p, TOK_DOT)) {
                strcat(imp->path, ".");  // Keep dot notation for module paths
                parser_advance(p);
            }
            if (parser_check(p, TOK_IDENT)) {
                strcat(imp->path, p->current.ident);
                parser_advance(p);
            }
        }
    } else if (parser_check(p, TOK_DOT)) {
        // Relative path ./something
        int i = 0;
        while (parser_check(p, TOK_DOT) || parser_check(p, TOK_SLASH) || parser_check(p, TOK_IDENT)) {
            if (parser_check(p, TOK_DOT)) { imp->path[i++] = '.'; parser_advance(p); }
            else if (parser_check(p, TOK_SLASH)) { imp->path[i++] = '/'; parser_advance(p); }
            else { 
                strcat(imp->path, p->current.ident); 
                i += strlen(p->current.ident);
                parser_advance(p); 
            }
        }
    }
    
    // Alias: import x as y
    if (parser_match(p, TOK_AS)) {
        Token alias = parser_expect(p, TOK_IDENT, "alias");
        strcpy(imp->alias, alias.ident);
    }
    
    return imp;
}

static Module* parse_module(Parser* p) {
    Module* m = calloc(1, sizeof(Module));
    m->imports = malloc(sizeof(Import) * 64);
    m->functions = malloc(sizeof(FnDef) * 256);
    m->structs = malloc(sizeof(StructDef) * 64);
    m->enums = malloc(sizeof(EnumDef) * 64);
    m->interfaces = malloc(sizeof(InterfaceDef) * 32);
    m->tests = malloc(sizeof(TestDef) * 128);
    m->top_level_stmts = malloc(sizeof(Stmt*) * 256);
    
    while (!parser_check(p, TOK_EOF)) {
        bool is_pub = parser_match(p, TOK_PUB);
        
        if (parser_check(p, TOK_IMPORT)) {
            m->imports[m->import_count++] = *parse_import(p);
        } else if (parser_check(p, TOK_FN)) {
            m->functions[m->function_count++] = *parse_function(p, is_pub, false);
        } else if (parser_check(p, TOK_STRUCT)) {
            m->structs[m->struct_count++] = *parse_struct(p, is_pub);
        } else if (parser_check(p, TOK_ENUM)) {
            m->enums[m->enum_count++] = *parse_enum(p, is_pub);
        } else if (parser_check(p, TOK_TEST)) {
            m->tests[m->test_count++] = *parse_test(p);
        } else {
            // Try to parse as a top-level statement
            if (!parser_check(p, TOK_EOF) && !parser_check(p, TOK_NEWLINE)) {
                Stmt* stmt = parse_stmt(p);
                if (stmt) {
                    m->top_level_stmts[m->top_level_count++] = stmt;
                }
            } else {
                parser_advance(p); // Skip newlines and unknown tokens
            }
        }
    }
    return m;
}

// AST printing for debugging
static void print_type(Type* t) {
    if (!t) { printf("?"); return; }
    switch (t->kind) {
        case TYPE_INT: printf("int"); break;
        case TYPE_FLOAT: printf("float"); break;
        case TYPE_STR: printf("str"); break;
        case TYPE_BOOL: printf("bool"); break;
        case TYPE_BYTE: printf("byte"); break;
        case TYPE_ANY: printf("any"); break;
        case TYPE_VOID: printf("void"); break;
        case TYPE_ARRAY: printf("["); print_type(t->inner); printf("]"); break;
        case TYPE_OPTIONAL: printf("?"); print_type(t->inner); break;
        case TYPE_NAMED: printf("%s", t->name); break;
        case TYPE_GENERIC: 
            printf("%s[", t->name);
            for (int i = 0; i < t->param_count; i++) {
                if (i > 0) printf(", ");
                print_type(t->params[i]);
            }
            printf("]");
            break;
        default: printf("???"); break;
    }
}

static void print_module(Module* m) {
    printf("\n=== AST ===\n");
