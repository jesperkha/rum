CC = gcc
FLAGS = -Wall -Wextra -Wpedantic -Werror -Og -g -Iinclude
OBJDIR = bin
TARGET = rum.exe

SRC = $(wildcard src/*.c) $(wildcard src/*/*.c) $(wildcard src/*/*/*.c)
OBJS = $(patsubst src/%, $(OBJDIR)/%, $(SRC:.c=.o))

all: $(TARGET)
	mkdir -p temp
	cp src/main.c temp/main.c

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) -o $@ $^ -DDEBUG

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	mkdir -p $(@D) && $(CC) $(FLAGS) -DDEBUG -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

release:
	gcc $(SRC) -Iinclude -DRELEASE -s -flto -O2 -o $(TARGET)

installer:
	python scripts/make_installer.py

debug:
	bash scripts/debug.sh

run: all
	catchsegv -- ./rum temp/main.c
	bat log

push:
	python scripts/version.py
	git add .
	git commit -m "updated version"
	git push origin dev

clean:
	rm -f *.exe *.zip gmon.out log
	rm -rf temp bin dist