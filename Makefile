SRC := $(wildcard src/*.c) $(wildcard src/*/*.c)
INCLUDE := -I src/core -I src/impl
GCC_BUILD = gcc $(SRC) $(INCLUDE) -o wim -Wall -Werror -std=c99

COMPILE_THEME = pushsd color && python convert.py && popd

build:
	$(COMPILE_THEME)
	$(GCC_BUILD)

debug:
	$(GCC_BUILD) -pg
	catchsegv -- ./wim $(ARGS)
	gprof wim.exe gmon.out --brief | less

clean:
	rm wim.exe gmon.out log