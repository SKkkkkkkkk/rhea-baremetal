#!/bin/bash

# 1. rm old cmake build files
find apps/ -type d -name "build" -exec rm -rf {} +

# 2. find all build.sh in apps/ and run them in parallel
find apps/ -name "build.sh" -print0 | xargs -0 -P $(nproc) -I {} sh -c '
  script_path="{}"
  script_dir=$(dirname "$script_path")
  cd "$script_dir" && ./build.sh >/dev/null 2>&1
  exit_code=$?
  if [ $exit_code -eq 0 ]; then
    echo "$script_path executed successfully"
  else
    printf "\033[1;31mError in $script_path (exit code: $exit_code)\033[0m\n"
    exit 1
  fi
'

