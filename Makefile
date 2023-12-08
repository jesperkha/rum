gcc_build = gcc src/*.c -I src -o wim -Wall -Wpedantic -Werror -std=c99

build:
	$(gcc_build)

debug:
	$(gcc_build) -pg
	catchsegv -- ./wim $(ARGS)
	gprof wim.exe gmon.out --brief | less

clean:
	rm wim.exe gmon.out log