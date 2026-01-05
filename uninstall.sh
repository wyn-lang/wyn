#!/bin/bash
# Wyn compiler uninstall script

if [ -f /usr/local/bin/wyn ]; then
    if [ -w /usr/local/bin ]; then
        rm /usr/local/bin/wyn
    else
        sudo rm /usr/local/bin/wyn
    fi
    echo "✅ Wyn compiler uninstalled"
else
    echo "⚠️  Wyn compiler not found in /usr/local/bin"
fi
