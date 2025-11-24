CC = gcc
CFLAGS = -Wall -Iinclude -g

APP = bin/piano_buddy

# 3. Source Files
SOURCES = src/main.c \
          src/recorder.c \
          src/recognizer.c \
          src/downloader.c \
          src/scraper_bridge.c \
          src/midi_player.c

OBJECTS = $(SOURCES:.c=.o)

# 4. Linker Flags
# -lportaudio -lsndfile (Recorder)
# -lcurl -lssl -lcrypto -ljansson (Recognizer & Downloader)
# -lfluidsynth (MIDI Player)
LDFLAGS = -lportaudio -lsndfile -lcurl -lssl -lcrypto -ljansson -lfluidsynth

# 5. Build Rules
all: $(APP)

$(APP): $(OBJECTS)
	@echo "🎹 Linking Piano Buddy..."
	$(CC) $(OBJECTS) -o $(APP) $(LDFLAGS)
	@echo "✅ Build successful! Run with: ./$(APP)"

# Generic compile rule
src/%.o: src/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -f src/*.o $(APP) recording.wav song.mid