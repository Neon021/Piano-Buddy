CC = gcc
CFLAGS = -Wall -Iinclude -g

APP = bin/piano_buddy_gui

SOURCES = src/gui_main.c \
          src/recorder.c \
          src/recognizer.c \
          src/processor_bridge.c \
		  src/library_manager.c \
		  src/jit_mixer.c

OBJECTS = $(SOURCES:.c=.o)

LDFLAGS = -lportaudio -lsndfile -lcurl -lssl -lcrypto -ljansson \
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
	rm -rf src/*.o $(APP) debug_scraper_dump.html recording.wav downloaded_song.mp3 backing_track.mp3 separated_output/