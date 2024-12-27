#!/bin/bash

# Check if at least one argument is passed
if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <file1> <file2> ... <fileN>"
    exit 1
fi

# Iterate over each file path provided as an argument
for filepath in "$@"; do
    if [ -f "$filepath" ]; then
        echo "====="
        echo "// $filepath"
        cat "$filepath"
    else
        echo "====="
        echo "// $filepath"
        echo "Error: File does not exist or is not a regular file."
    fi
done