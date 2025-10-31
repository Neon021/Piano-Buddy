# 1. Compiler
CC = gcc
CFLAGS = -Wall -Iinclude -g

# 2. Executables
REC_APP = bin/record_app
RECOGNIZE_APP = bin/recognize_app
MIDI_APP = bin/midi_player_app

# 3. Source & Object files
REC_SRC = src/recorder.c
REC_OBJ = $(REC_SRC:.c=.o)

RECOGNIZE_SRC = src/recognizer.c
RECOGNIZE_OBJ = $(RECOGNIZE_SRC:.c=.o)

MIDI_SRC = src/midi_player.c
MIDI_OBJ = $(MIDI_SRC:.c=.o)

# 4. Linker Flags (Libraries)
LDFLAGS_REC = -lportaudio -lsndfile
LDFLAGS_RECOGNIZE = -lcurl -lssl -lcrypto -ljansson
LDFLAGS_MIDI = -lfluidsynth

# 5. Build Rules
all: $(REC_APP) $(RECOGNIZE_APP) $(MIDI_APP)

# Rule to build the recorder
$(REC_APP): $(REC_OBJ)
	@echo "Linking Recorder..."
	$(CC) $(REC_OBJ) -o $(REC_APP) $(LDFLAGS_REC)
	@echo "Recorder build finished: $(REC_APP)"

# Rule to build the recognizer
$(RECOGNIZE_APP): $(RECOGNIZE_OBJ)
	@echo "Linking Recognizer..."
	$(CC) $(RECOGNIZE_OBJ) -o $(RECOGNIZE_APP) $(LDFLAGS_RECOGNIZE)
	@echo "Recognizer build finished: $(RECOGNIZE_APP)"

# Rule to build the MIDI player
$(MIDI_APP): $(MIDI_OBJ)
	@echo "Linking MIDI Player..."
	$(CC) $(MIDI_OBJ) -o $(MIDI_APP) $(LDFLAGS_MIDI)
	@echo "MIDI Player build finished: $(MIDI_APP)"

# Generic rule to compile any .c file in src/ into its .o file
src/%.o: src/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up
clean:
	@echo "Cleaning up..."
	rm -f src/*.o $(REC_APP) $(RECOGNIZE_APP) $(MIDI_APP)