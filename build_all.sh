# 1. rm old cmake build files
find apps/ -maxdepth 3 -type f -name "CMakeCache.txt" -exec rm -rf {} +
find apps/ -maxdepth 3 -type d -name "CMakeFiles" -exec rm -rf {} +

# 2. find all build.sh in apps/ and run them
find apps/ -name "build.sh" -execdir sh -c 'if ./{} >/dev/null; then echo "$(pwd)/{} executed successfully"; else echo "\033[1;31mError occurred while executing $(pwd)/{}\033[0m"; exit 1; fi' \;
