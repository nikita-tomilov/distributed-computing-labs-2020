SRC := .
OBJ := obj
INC := .

PROJ := lab1pa4

CC  := gcc
CFLAGS := -Wall -g -std=c99 -Wpedantic -Werror

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

all: objdir program

objdir:
	mkdir -p $(OBJ)

program: $(OBJECTS)
	$(CC) -L../dylib $^ -o $(PROJ) -lruntime

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -I$(INC) -c $< $(CFLAGS) -o $@

clean:
	rm -rf $(OBJ) $(PROJ)
