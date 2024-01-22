SRC := $(wildcard src/*.c) $(wildcard src/*/*.c)
INCLUDE := $(addprefix -I, $(dir $(wildcard src/*/)))

GCC_BUILD = gcc $(SRC) $(INCLUDE) -o wim -Wall -Werror -std=c99

build:
	mkdir -p temp
	python scripts/gen.py
	$(GCC_BUILD)

debug:
	$(GCC_BUILD) -pg
	catchsegv -- ./wim $(ARGS)
	gprof wim.exe gmon.out --brief | less

clean:
	rm -f wim.exe gmon.out log
	rm -rf temp