#!/usr/bin/env bash
# Platform detection and LLVM path finder for Wyn builds

set -e

detect_platform() {
    local os=$(uname -s 2>/dev/null || echo "Windows")
    local arch=$(uname -m 2>/dev/null || echo "x86_64")
    
    case "$os" in
        Darwin)
            if [ "$arch" = "arm64" ]; then
                echo "macos-arm64"
            else
                echo "macos-x86_64"
            fi
            ;;
        Linux)
            if [ "$arch" = "aarch64" ]; then
                echo "linux-arm64"
            else
                echo "linux-x86_64"
            fi
            ;;
        MINGW*|MSYS*|CYGWIN*|Windows*)
            echo "windows-x86_64"
            ;;
        *)
            echo "unknown"
            ;;
    esac
}

find_llvm() {
    local platform=$(detect_platform)
    
    case "$platform" in
        macos-arm64)
            if [ -d "/opt/homebrew/opt/llvm" ]; then
                echo "/opt/homebrew/opt/llvm"
            elif [ -d "/opt/homebrew/opt/llvm@18" ]; then
                echo "/opt/homebrew/opt/llvm@18"
            fi
            ;;
        macos-x86_64)
            if [ -d "/usr/local/opt/llvm" ]; then
                echo "/usr/local/opt/llvm"
            elif [ -d "/usr/local/opt/llvm@18" ]; then
                echo "/usr/local/opt/llvm@18"
            fi
            ;;
        linux-*)
            for version in 18 17 16 15 14; do
                if [ -d "/usr/lib/llvm-$version" ]; then
                    echo "/usr/lib/llvm-$version"
                    return
                fi
            done
            ;;
        windows-*)
            if [ -d "/c/Program Files/LLVM" ]; then
                echo "C:/Program Files/LLVM"
            fi
            ;;
    esac
}

verify_llvm() {
    local llvm_prefix="$1"
    
    if [ -z "$llvm_prefix" ]; then
        echo "Error: LLVM not found" >&2
        return 1
    fi
    
    if [ ! -d "$llvm_prefix" ]; then
        echo "Error: LLVM directory does not exist: $llvm_prefix" >&2
        return 1
    fi
    
    local llvm_config="$llvm_prefix/bin/llvm-config"
    if [ ! -f "$llvm_config" ] && [ ! -f "$llvm_config.exe" ]; then
        echo "Error: llvm-config not found in $llvm_prefix/bin" >&2
        return 1
    fi
    
    return 0
}

case "${1:-detect}" in
    detect)
        detect_platform
        ;;
    find-llvm)
        find_llvm
        ;;
    verify-llvm)
        verify_llvm "$2"
        ;;
    info)
        platform=$(detect_platform)
        llvm_path=$(find_llvm)
        echo "Platform: $platform"
        echo "LLVM Path: ${llvm_path:-not found}"
        if [ -n "$llvm_path" ]; then
            verify_llvm "$llvm_path" && echo "LLVM Status: OK" || echo "LLVM Status: Invalid"
        fi
        ;;
    *)
        echo "Usage: $0 {detect|find-llvm|verify-llvm <path>|info}"
        exit 1
        ;;
esac
