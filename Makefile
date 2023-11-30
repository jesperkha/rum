build: src/editor.c
	gcc src/*.c -I src -o wim -Wall -Wpedantic -Werror -std=c99 -g

clean:
	rm wim.exe