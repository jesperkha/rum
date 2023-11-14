build: wim.c
	gcc wim.c -o wim -Wall -Wpedantic -Werror -std=c99 -g

clean:
	rm wim.exe