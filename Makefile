CC = gcc
FLAGS = -Wall -g -Iinclude
SRC = $(wildcard src/*.c) $(wildcard src/*/*.c)
OBJDIR = bin
OBJS = $(patsubst src/%, $(OBJDIR)/%, $(SRC:.c=.o))
TARGET = rum.exe

all: $(TARGET)
	mkdir -p temp
	cp src/main.c temp/main.c

$(TARGET): $(OBJS)
	$(CC) $(FLAGS) -o $@ $^ -DDEBUG

$(OBJDIR)/%.o: src/%.c | $(OBJDIR)
	mkdir -p $(@D) && $(CC) $(FLAGS) -DDEBUG -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

tcc:
	tcc $(SRC) $(FLAGS) -o $(TARGET) -DDEBUG[=1]

release:
	gcc $(SRC) -Iinclude -s -flto -O2 -o $(TARGET)

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
	rm -f $(TARGET) gmon.out log
	rm -rf temp bin