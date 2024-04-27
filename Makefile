SRC := $(wildcard src/*.c) $(wildcard src/*/*.c)
INCLUDE := $(addprefix -I, $(dir $(wildcard src/*/)))

GCC_BUILD = gcc $(SRC) $(INCLUDE) -o wim -Wall -Werror -std=c99
TCC_BUILD = tcc $(SRC) $(INCLUDE) -o wim.exe -g -DDEBUG[=1]

tcc: .scripts
	$(TCC_BUILD)

gcc: .scripts
	$(GCC_BUILD) -D DEBUG

debug: .scripts
	$(GCC_BUILD) -D DEBUG -pg
	catchsegv -- ./wim $(ARGS)
	gprof wim.exe gmon.out --brief | less

release: .scripts
	$(GCC_BUILD) -s -flto -O2

clean:
	rm wim.exe
	rm -f gmon.out log
	rm -rf temp

.scripts:
	mkdir -p temp
	python scripts/gen.py