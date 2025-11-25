CC = gcc
CFLAGS = -Wall -Iinclude -g

APP = bin/piano_buddy_gui

SOURCES = src/gui_main.c \
          src/recorder.c \
          src/recognizer.c \
          src/downloader.c \
          src/scraper_bridge.c \
          src/midi_player.c

OBJECTS = $(SOURCES:.c=.o)

LDFLAGS = -lportaudio -lsndfile -lcurl -lssl -lcrypto -ljansson -lfluidsynth \
          -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all: $(APP)

$(APP): $(OBJECTS)
	@echo "🎹 Linking Piano Buddy GUI..."
	$(CC) $(OBJECTS) -o $(APP) $(LDFLAGS)
	@echo "✅ Build successful! Run with: ./$(APP)"

src/%.o: src/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -f src/*.o $(APP) recording.wav song.mid