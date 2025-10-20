CC = gcc

CFLAGS = -Wall -Iinclude -g

REC_APP = bin/record_app
RECOGNIZE_APP = bin/recognize_app

REC_SRC = src/recorder.c
REC_OBJ = $(REC_SRC:.c=.o)

RECOGNIZE_SRC = src/recognizer.c
RECOGNIZE_OBJ = $(RECOGNIZE_SRC:.c=.o)

LDFLAGS_REC = -lportaudio -lsndfile
# Link against curl, ssl, crypto (for openssl), and jansson
LDFLAGS_RECOGNIZE = -lcurl -lssl -lcrypto -ljansson

# Build Rules
all: $(REC_APP) $(RECOGNIZE_APP)

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

# Generic rule to compile any .c file in src/ into its .o file
src/%.o: src/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up
clean:
	@echo "Cleaning up..."
	rm -f src/*.o $(REC_APP) $(RECOGNIZE_APP)