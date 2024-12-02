#!/bin/bash

set -e

if ! command -v run-clang-tidy &> /dev/null; then
  echo "Error: run-clang-tidy not found. Please ensure it is in your PATH."
  exit 1
fi

find . -type f \( -name "*.c" -o -name "*.h" \) \
  -not -path "./external/*" -not -path "./build/*" -print0 | \
  xargs -0 run-clang-tidy -p . -header-filter=".*" \
  -checks="-*,clang-analyzer-*,bugprone-*,cppcoreguidelines-*,google-*,modernize-*,performance-*" \
