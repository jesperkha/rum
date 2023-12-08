build: src/editor.c
	gcc src/*.c -I src -o wim -Wall -Wpedantic -Werror -std=c99 -g
	catchsegv -- ./wim

clean:
	rm wim.exe