#!/bin/bash

# Check if a directory name was provided
if [ -z "$1" ]; then
    echo "Usage: ./snapshot.sh <project_folder_name>"
    exit 1
fi

PROJECT_DIR=$1
OUTPUT_FILE="${PROJECT_DIR}_snapshot.txt"

# Verify the directory exists
if [ ! -d "$PROJECT_DIR" ]; then
    echo "Error: Directory '$PROJECT_DIR' not found."
    exit 1
fi

{
  echo "=== SNAPSHOT DATE: $(date) ==="
  echo "=== PROJECT ROOT: $PROJECT_DIR ==="
  echo -e "\n=== DIRECTORY TREE ==="
  tree "$PROJECT_DIR"

  echo -e "\n=== FILE CONTENTS ==="

  # Expanded to include .conf, .qml, and .json
# Expanded to include .conf, .qml, .json, and your read.me
  find "$PROJECT_DIR" -type f \( -name "*.c" -o -name "*.h" -o -name "Makefile" -o -name "*.sh" -o -name "*.conf" -o -name "*.qml" -o -name "*.json" -o -name "read.me" -o -name "README*" -o -name "*.txt" \) | while read -r file; do
    echo "----------------------------------------------------------------"
    echo "FILE: $file"
    echo "----------------------------------------------------------------"

    # Determine code block type for proper parsing
    case "$file" in
        *.c|*.h)       LANG="c" ;;
        *.sh)          LANG="bash" ;;
        *.qml)         LANG="qml" ;;
        *.json|*.conf) LANG="json" ;;
        *read.me|*README*|*.txt) LANG="text" ;; # Handled as plain text
        *)             LANG="makefile" ;;
    esac

    echo "\`\`\`$LANG"
    cat "$file"
    echo "\`\`\`"
    echo -e "\n"
  done
} > "$OUTPUT_FILE"

echo "Success: $OUTPUT_FILE created."
