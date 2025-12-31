#!/usr/bin/env python3
# Final curated list of examples to keep (~50 essential examples)

KEEP_FILES = [
    # Core Basics (8)
    "hello.wyn",
    "variables.wyn", 
    "functions.wyn",
    "arithmetic.wyn",
    "conditionals.wyn",
    "loops.wyn",
    "recursion.wyn",
    "fibonacci.wyn",  # Classic example
    
    # Data Structures & Operations (8)
    "array_operations.wyn",
    "string_operations.wyn", 
    "struct_methods.wyn",
    "list_comprehension.wyn",
    "destructuring_demo.wyn",
    "slice_demo.wyn",
    "array_methods_demo.wyn",
    "string_interpolation_demo.wyn",
    
    # Unique Language Features (8)
    "extension_methods.wyn",
    "closures_demo.wyn",
    "traits_demo.wyn",
    "async_await_demo.wyn",
    "spawn_demo.wyn",
    "method_chaining_demo.wyn",
    "pipe_operator_demo.wyn",
    "ternary_demo.wyn",
    
    # Error Handling (3)
    "error_handling_demo.wyn",
    "result_option_demo.wyn", 
    "advanced_error_handling.wyn",
    
    # Real Use Cases (10)
    "calculator.wyn",
    "fizzbuzz.wyn",
    "http_server.wyn",
    "fs_demo.wyn",
    "devops_automation.wyn",
    "file_finder.wyn",
    "shell_replacement_demo.wyn",
    "database_demo.wyn",
    "infrastructure_demo.wyn",
    "production_example.wyn",
    
    # Testing & Development (3)
    "testing_demo.wyn",
    "args_demo.wyn",
    "using_packages.wyn",
    
    # Advanced Examples (5)
    "complete.wyn",
    "showcase.wyn",
    "feature_complete_demo.wyn",
    "comprehensive.wyn",
    "ultimate_demo.wyn",
    
    # Concurrency Examples (3)
    "spawn_simple.wyn",
    "spawn_million.wyn",
    "command_chaining.wyn",
    
    # String & I/O (2)
    "multiline_string_demo.wyn",
    "io_demo.wyn"
]

print(f"Total files to keep: {len(KEEP_FILES)}")
for i, file in enumerate(KEEP_FILES, 1):
    print(f"{i:2d}. {file}")