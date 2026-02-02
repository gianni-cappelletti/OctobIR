#!/usr/bin/env bash
# Display OctobIR ASCII art header with colored logo

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
HEADER_FILE="$PROJECT_DIR/scripts/header.txt"

# ANSI color codes
ORANGE='\033[38;5;208m'
RESET='\033[0m'

# Check if header file exists
if [ ! -f "$HEADER_FILE" ]; then
  echo "Warning: Header file not found at $HEADER_FILE"
  exit 0
fi

# Read and print the header file with orange color
while IFS= read -r line; do
  printf "${ORANGE}%s${RESET}\n" "$line"
done < "$HEADER_FILE"