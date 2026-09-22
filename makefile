# Projeto 1 - Processamento de imagens
# Compilacao: make        (gera build/proj1 e copia assets/)
#             make clean  (remove build/)

TARGET  := proj1
SRC_DIR := src
OUT_DIR := build

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OUT_DIR)/%.o,$(SOURCES))

CFLAGS  := -std=c99 -Wall -Wextra -O2
LDFLAGS :=
LDLIBS  := -lm

# Windows (MinGW): informe o caminho da SDL3 em SDL_PREFIX, por exemplo
#   mingw32-make SDL_PREFIX=C:/dev/libs/SDL3
ifdef SDL_PREFIX
  CFLAGS  += -I$(SDL_PREFIX)/include
  LDFLAGS += -L$(SDL_PREFIX)/lib
  LDLIBS  += -lSDL3 -lSDL3_image -lSDL3_ttf
else
  CFLAGS  += $(shell pkg-config --cflags sdl3 sdl3-image sdl3-ttf)
  LDLIBS  += $(shell pkg-config --libs sdl3 sdl3-image sdl3-ttf)
endif

ifeq ($(OS),Windows_NT)
  BIN := $(OUT_DIR)/$(TARGET).exe
else
  BIN := $(OUT_DIR)/$(TARGET)
endif

.PHONY: all clean

all: $(BIN) $(OUT_DIR)/assets

$(BIN): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OUT_DIR)/%.o: $(SRC_DIR)/%.c | $(OUT_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# A fonte precisa ficar ao lado do executavel: o programa monta o caminho
# a partir de SDL_GetBasePath().
$(OUT_DIR)/assets: assets | $(OUT_DIR)
	cp -r assets $(OUT_DIR)/

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

clean:
	rm -rf $(OUT_DIR)
