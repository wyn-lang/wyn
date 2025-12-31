#!/usr/bin/env python3
import os
import re
from pathlib import Path

def categorize_examples():
    examples_dir = Path("/Users/aoaws/src/ao/wyn-lang/wyn/examples")
    
    # Categories for keeping
    categories = {
        "core_basics": [],      # hello, variables, functions, arithmetic
        "control_flow": [],     # conditionals, loops, recursion
        "unique_features": [],  # extension_methods, comprehensions, async
        "real_use_cases": [],   # file_ops, http, devops, calculator
        "data_structures": [],  # arrays, strings, structs
        "error_handling": [],   # result, option, error handling
        "concurrency": [],      # spawn, async
        "testing": [],          # testing framework examples
        
        # Categories for removal
        "test_files": [],       # *_test.wyn files
        "stage1_files": [],     # stage1_* files  
        "duplicates": [],       # similar functionality
        "minimal_examples": [], # too simple/redundant
        "broken": []            # broken or outdated
    }
    
    for file_path in examples_dir.rglob("*.wyn"):
        filename = file_path.name
        rel_path = str(file_path.relative_to(examples_dir))
        
        # Skip wip directory for now
        if "wip/" in rel_path:
            continue
            
        # Categorize based on filename patterns
        if filename.startswith("stage1_"):
            categories["stage1_files"].append(rel_path)
        elif filename.endswith("_test.wyn") or "test" in filename:
            categories["test_files"].append(rel_path)
        elif filename in ["hello.wyn"]:
            categories["core_basics"].append(rel_path)
        elif filename in ["variables.wyn", "arithmetic.wyn", "functions.wyn"]:
            categories["core_basics"].append(rel_path)
        elif filename in ["conditionals.wyn", "loops.wyn", "recursion.wyn"]:
            categories["control_flow"].append(rel_path)
        elif filename in ["extension_methods.wyn", "list_comprehension.wyn", "destructuring_demo.wyn"]:
            categories["unique_features"].append(rel_path)
        elif filename in ["async_await_demo.wyn", "spawn_demo.wyn", "closures_demo.wyn"]:
            categories["concurrency"].append(rel_path)
        elif filename in ["fs_demo.wyn", "http_server.wyn", "devops_automation.wyn", "calculator.wyn"]:
            categories["real_use_cases"].append(rel_path)
        elif filename in ["array_operations.wyn", "string_operations.wyn", "struct_methods.wyn"]:
            categories["data_structures"].append(rel_path)
        elif filename in ["error_handling_demo.wyn", "result_option_demo.wyn"]:
            categories["error_handling"].append(rel_path)
        elif filename in ["testing_demo.wyn"]:
            categories["testing"].append(rel_path)
        elif filename in ["simple.wyn", "test_simple.wyn", "custom.wyn", "ast_test.wyn"]:
            categories["minimal_examples"].append(rel_path)
        else:
            # Need manual review
            categories["duplicates"].append(rel_path)
    
    return categories

def print_audit_results(categories):
    print("=== EXAMPLES AUDIT RESULTS ===\n")
    
    keep_count = 0
    remove_count = 0
    
    print("FILES TO KEEP:")
    for category in ["core_basics", "control_flow", "unique_features", "real_use_cases", 
                     "data_structures", "error_handling", "concurrency", "testing"]:
        if categories[category]:
            print(f"\n{category.upper().replace('_', ' ')} ({len(categories[category])}):")
            for file in sorted(categories[category]):
                print(f"  ✓ {file}")
            keep_count += len(categories[category])
    
    print(f"\nTOTAL TO KEEP: {keep_count}")
    
    print("\n" + "="*50)
    print("FILES TO REMOVE:")
    
    for category in ["test_files", "stage1_files", "minimal_examples", "duplicates"]:
        if categories[category]:
            print(f"\n{category.upper().replace('_', ' ')} ({len(categories[category])}):")
            for file in sorted(categories[category]):
                print(f"  ✗ {file}")
            remove_count += len(categories[category])
    
    print(f"\nTOTAL TO REMOVE: {remove_count}")
    print(f"CURRENT TOTAL: {keep_count + remove_count}")

if __name__ == "__main__":
    categories = categorize_examples()
    print_audit_results(categories)