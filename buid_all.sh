# find all build.sh in apps/ and run them
find apps/ -name "build.sh" -execdir sh -c 'if ./{} >/dev/null; then echo "$(pwd)/{} executed successfully"; else echo "\033[1;31mError occurred while executing $(pwd)/{}\033[0m"; exit 1; fi' \;
