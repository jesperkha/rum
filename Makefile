build: wim.c
	gcc wim.c -o wim -Wall -Wpedantic -Werror -std=c99

clean:
	rm wim.exe