SRC := $(wildcard src/*.c) $(wildcard src/*/*.c)
INCLUDE := $(addprefix -I, $(dir $(wildcard src/*/)))

GCC_BUILD = gcc $(SRC) $(INCLUDE) -o wim -Wall -Werror -std=c99

build: .scripts
	mkdir -p temp
	$(GCC_BUILD) -D DEBUG

debug:
	$(GCC_BUILD) -D DEBUG -pg
	catchsegv -- ./wim $(ARGS)
	gprof wim.exe gmon.out --brief | less

release: .clean_temp .scripts
	$(GCC_BUILD)

clean: .clean_temp
	rm wim.exe

.scripts:
	python scripts/gen.py

.clean_temp:
	rm -f gmon.out log
	rm -rf temp