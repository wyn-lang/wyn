#!/usr/bin/env python3
import os
import shutil
from pathlib import Path

# Files to keep (50 essential examples)
KEEP_FILES = {
    "hello.wyn", "variables.wyn", "functions.wyn", "arithmetic.wyn",
    "conditionals.wyn", "loops.wyn", "recursion.wyn", "fibonacci.wyn",
    "array_operations.wyn", "string_operations.wyn", "struct_methods.wyn",
    "list_comprehension.wyn", "destructuring_demo.wyn", "slice_demo.wyn",
    "array_methods_demo.wyn", "string_interpolation_demo.wyn",
    "extension_methods.wyn", "closures_demo.wyn", "traits_demo.wyn",
    "async_await_demo.wyn", "spawn_demo.wyn", "method_chaining_demo.wyn",
    "pipe_operator_demo.wyn", "ternary_demo.wyn", "error_handling_demo.wyn",
    "result_option_demo.wyn", "advanced_error_handling.wyn", "calculator.wyn",
    "fizzbuzz.wyn", "http_server.wyn", "fs_demo.wyn", "devops_automation.wyn",
    "file_finder.wyn", "shell_replacement_demo.wyn", "database_demo.wyn",
    "infrastructure_demo.wyn", "production_example.wyn", "testing_demo.wyn",
    "args_demo.wyn", "using_packages.wyn", "complete.wyn", "showcase.wyn",
    "feature_complete_demo.wyn", "comprehensive.wyn", "ultimate_demo.wyn",
    "spawn_simple.wyn", "spawn_million.wyn", "command_chaining.wyn",
    "multiline_string_demo.wyn", "io_demo.wyn"
}

def cleanup_examples():
    examples_dir = Path("/Users/aoaws/src/ao/wyn-lang/wyn/examples")
    
    # Create backup directory
    backup_dir = examples_dir / "removed_examples_backup"
    backup_dir.mkdir(exist_ok=True)
    
    removed_count = 0
    kept_count = 0
    
    print("Starting cleanup...")
    
    # Process all .wyn files
    for file_path in examples_dir.rglob("*.wyn"):
        filename = file_path.name
        rel_path = str(file_path.relative_to(examples_dir))
        
        # Skip files in wip directory (handle separately)
        if "wip/" in rel_path:
            continue
            
        if filename in KEEP_FILES:
            print(f"✓ KEEPING: {rel_path}")
            kept_count += 1
        else:
            # Move to backup directory
            backup_path = backup_dir / rel_path
            backup_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(str(file_path), str(backup_path))
            print(f"✗ REMOVED: {rel_path}")
            removed_count += 1
    
    # Remove empty directories
    for dir_path in examples_dir.rglob("*"):
        if dir_path.is_dir() and dir_path != backup_dir:
            try:
                if not any(dir_path.iterdir()):
                    dir_path.rmdir()
                    print(f"✗ REMOVED EMPTY DIR: {dir_path.relative_to(examples_dir)}")
            except OSError:
                pass  # Directory not empty
    
    print(f"\nCleanup complete!")
    print(f"Files kept: {kept_count}")
    print(f"Files removed: {removed_count}")
    print(f"Backup location: {backup_dir}")
    
    return kept_count, removed_count

if __name__ == "__main__":
    cleanup_examples()