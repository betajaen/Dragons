emcc main.c -g -O2 -s USE_SDL=2 --embed-file assets --shell-file emscriptem_shell.html -o bin/game.html
